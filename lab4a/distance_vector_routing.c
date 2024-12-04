#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>

#define PORT 8080
#define MAX_MESSAGE_LENGTH 100
#define BROADCAST_IP "255.255.255.255"
#define NEIGHBOUR_TIMEOUT 10
#define IP_SIZE 256
#define MAX_NEIGHBORS 256
#define MAX_DESTINATIONS 256

typedef struct {
    char ipAddress[IP_SIZE];
    unsigned short sequenceNumber;
    time_t lastReceived;
} Neighbour;

typedef struct {
    char destination[IP_SIZE];
    int distance;
} DistanceEntry;

typedef struct {
    char destination[IP_SIZE];
    DistanceEntry distances[MAX_NEIGHBORS];
    int numDistances;
} DistanceVectorEntry;

pthread_mutex_t neighbourTableMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t distanceVectorMutex = PTHREAD_MUTEX_INITIALIZER;

Neighbour neighbourTable[MAX_NEIGHBORS];
int neighbourCount = 0;

DistanceVectorEntry distanceVector[MAX_DESTINATIONS];
int distanceVectorCount = 0;
bool updatedDV = false;
char* myIPAddress;

void findMyIP(){//https://www.sanfoundry.com/c-program-get-ip-address/
    int n;
    struct ifreq ifr;
    char array[] = "eth0";

    n = socket(AF_INET, SOCK_DGRAM, 0);

    //Type of address to retrieve - IPv4 IP address
    ifr.ifr_addr.sa_family = AF_INET;

    //Copy the interface name in the ifreq structure
    strncpy(ifr.ifr_name , array , sizeof(ifr.ifr_name));
    ioctl(n, SIOCGIFADDR, &ifr);
    close(n);

    myIPAddress = (char *)calloc(IP_SIZE, sizeof(char));
    strcpy (myIPAddress, inet_ntoa(( (struct sockaddr_in *)&ifr.ifr_addr )->sin_addr));
    return;
}

void* sendHelloMessage(void* vargp) {
    int sockfd;
    struct sockaddr_in broadcast_addr;
    char message[MAX_MESSAGE_LENGTH];
    unsigned short sequenceNumber = 0;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    int broadcast = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0) {
        perror("Setting socket options failed");
        exit(EXIT_FAILURE);
    }

    memset(&broadcast_addr, 0, sizeof(broadcast_addr));
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_port = htons(PORT);
    broadcast_addr.sin_addr.s_addr = inet_addr(BROADCAST_IP);

    while (true) {
        snprintf(message, sizeof(message), "%s:HELLO:%hu", myIPAddress, htons(sequenceNumber++));
        sendto(sockfd, message, strlen(message), 0, (struct sockaddr*)&broadcast_addr, sizeof(broadcast_addr));
        printf("Broadcasting message: %s\n", message);
        sleep(5);
    }

    close(sockfd);
    return NULL;
}

void* processIncomingHello(void* vargp) {
    int sockfd;
    struct sockaddr_in recv_addr;
    char buffer[MAX_MESSAGE_LENGTH];
    socklen_t addr_len = sizeof(recv_addr);

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&recv_addr, 0, sizeof(recv_addr));
    recv_addr.sin_family = AF_INET;
    recv_addr.sin_port = htons(PORT);
    recv_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (const struct sockaddr*)&recv_addr, sizeof(recv_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    while (true) {
        int n = recvfrom(sockfd, buffer, MAX_MESSAGE_LENGTH, 0, (struct sockaddr*)&recv_addr, &addr_len);
        buffer[n] = '\0';
        printf("Received message: %s\n", buffer);

        char senderIP[IP_SIZE];
        unsigned short receivedSeqNum;

        sscanf(buffer, "%[^:]:HELLO:%hu", senderIP, &receivedSeqNum);

        if (strcmp(senderIP, myIPAddress) == 0) {
            continue;
        }

        pthread_mutex_lock(&neighbourTableMutex);
        bool found = false;
        for (int i = 0; i < neighbourCount; i++) {
            if (strcmp(neighbourTable[i].ipAddress, senderIP) == 0) {
                if (ntohs(receivedSeqNum) > ntohs(neighbourTable[i].sequenceNumber)) {
                    neighbourTable[i].sequenceNumber = receivedSeqNum;
                    neighbourTable[i].lastReceived = time(NULL);
                }
                found = true;
                break;
            }
        }
        if (!found && neighbourCount < MAX_NEIGHBORS) {
            strcpy(neighbourTable[neighbourCount].ipAddress, senderIP);
            neighbourTable[neighbourCount].sequenceNumber = receivedSeqNum;
            neighbourTable[neighbourCount].lastReceived = time(NULL);
            neighbourCount++;
            printf("New neighbour detected: %s\n", senderIP);
        }
        pthread_mutex_unlock(&neighbourTableMutex);
    }

    close(sockfd);
    return NULL;
}

void* monitorNeighbours(void* vargp) {
    while (true) {
        pthread_mutex_lock(&neighbourTableMutex);
        time_t currentTime = time(NULL);
        for (int i = 0; i < neighbourCount; i++) {
            if (difftime(currentTime, neighbourTable[i].lastReceived) > NEIGHBOUR_TIMEOUT) {
                printf("Neighbour lost: %s\n", neighbourTable[i].ipAddress);
                for (int j = i; j < neighbourCount - 1; j++) {
                    neighbourTable[j] = neighbourTable[j + 1];
                }
                neighbourCount--;
                i--;
            }
        }
        pthread_mutex_unlock(&neighbourTableMutex);
        sleep(1);
    }
    return NULL;
}

void* printNeighbourTable(void* vargp) { 
    while (true) { 
        pthread_mutex_lock(&neighbourTableMutex); 
        printf("\nNeighbour Table:\n"); 
        for (int i = 0; i < neighbourCount; i++) { 
            printf("Neighbour IP: %s, Sequence Number: %hu, Last Received: %ld seconds ago\n", neighbourTable[i].ipAddress, ntohs(neighbourTable[i].sequenceNumber), time(NULL) - neighbourTable[i].lastReceived); 
        }
        printf("\n-------------\n"); 
        pthread_mutex_unlock(&neighbourTableMutex); 
        sleep(5);
    }
    return NULL;
}

// Function to get the distance vector as a string
char* getDistanceVector() {
    pthread_mutex_lock(&distanceVectorMutex);
    char* dvString = (char*)malloc(MAX_MESSAGE_LENGTH * sizeof(char));
    strcpy(dvString, "DV:");

    for (int i = 0; i < distanceVectorCount; i++) {
        for (int j = 0; j < distanceVector[i].numDistances; j++) {
            char entry[IP_SIZE + 10];
            snprintf(entry, sizeof(entry), "(%s,%d):", distanceVector[i].destination, distanceVector[i].distances[j].distance);
            strcat(dvString, entry);
        }
    }
    
    pthread_mutex_unlock(&distanceVectorMutex);
    return dvString;
}

void processDistanceVector(char* DV) {
    pthread_mutex_lock(&distanceVectorMutex);
    char* token = strtok(DV, ":");
    while (token != NULL) {
        if (strncmp(token, "DV", 2) != 0) {
            char dest[IP_SIZE];
            int dist;
            sscanf(token, "(%[^,],%d)", dest, &dist);
            bool found = false;
            for (int i = 0; i < distanceVectorCount; i++) {
                if (strcmp(distanceVector[i].destination, dest) == 0) {
                    found = true;
                    for (int j = 0; j < distanceVector[i].numDistances; j++) {
                        if (distanceVector[i].distances[j].distance > dist) {
                            distanceVector[i].distances[j].distance = dist;
                            updatedDV = true;
                            break;
                        }
                    }
                }
            }
            if (!found && distanceVectorCount < MAX_DESTINATIONS) {
                strcpy(distanceVector[distanceVectorCount].destination, dest);
                distanceVector[distanceVectorCount].distances[0].distance = dist;
                distanceVector[distanceVectorCount].numDistances = 1;
                distanceVectorCount++;
                updatedDV = true;
            }
        }
        token = strtok(NULL, ":");
    }
    pthread_mutex_unlock(&distanceVectorMutex);
}

// Integration function to handle DV update
void dvUpdate() {
    updatedDV = true;
}

// Integration function to handle DV sent
void dvSent() {
    pthread_mutex_lock(&distanceVectorMutex);
    updatedDV = false;
    pthread_mutex_unlock(&distanceVectorMutex);
}

void* broadcastDistanceVector(void* vargp) {
    while (true) {
        sleep(5); // Periodic check
        pthread_mutex_lock(&distanceVectorMutex);
        if (updatedDV) {
            char* dvString = getDistanceVector();

            int sockfd;
            struct sockaddr_in broadcast_addr;

            if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
                perror("Socket creation failed");
                pthread_mutex_unlock(&distanceVectorMutex);
                exit(EXIT_FAILURE);
            }

            int broadcast = 1;
            if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0) {
                perror("Setting socket options failed");
                close(sockfd);
                pthread_mutex_unlock(&distanceVectorMutex);
                exit(EXIT_FAILURE);
            }

            memset(&broadcast_addr, 0, sizeof(broadcast_addr));
            broadcast_addr.sin_family = AF_INET;
            broadcast_addr.sin_port = htons(PORT);
            broadcast_addr.sin_addr.s_addr = inet_addr(BROADCAST_IP);

            sendto(sockfd, dvString, strlen(dvString), 0, (struct sockaddr*)&broadcast_addr, sizeof(broadcast_addr));
            printf("Broadcasting Distance Vector: %s\n", dvString);

            free(dvString);
            close(sockfd);

            updatedDV = false;
        }
        pthread_mutex_unlock(&distanceVectorMutex);
    }
    return NULL;
}

void* processIncomingDV(void* vargp) {
    int sockfd;
    struct sockaddr_in recv_addr;
    char buffer[MAX_MESSAGE_LENGTH];
    socklen_t addr_len = sizeof(recv_addr);

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&recv_addr, 0, sizeof(recv_addr));
    recv_addr.sin_family = AF_INET;
    recv_addr.sin_port = htons(PORT);
    recv_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (const struct sockaddr*)&recv_addr, sizeof(recv_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    while (true) {
        int n = recvfrom(sockfd, buffer, MAX_MESSAGE_LENGTH, 0, (struct sockaddr*)&recv_addr, &addr_len);
        buffer[n] = '\0';
        printf("Received message: %s\n", buffer);

        if (strncmp(buffer, myIPAddress, strlen(myIPAddress)) == 0) {
            continue; // Ignore self-sent messages
        }

        if (strstr(buffer, ":DV:") != NULL) {
            char* dvMessage = strchr(buffer, ':') + 1; // Extract DV part
            processDistanceVector(dvMessage);
        }
    }

    close(sockfd);
    return NULL;
}

void* printDistanceVectorTable(void* vargp) { 
    while (true) { 
        pthread_mutex_lock(&distanceVectorMutex); 
        printf("\nDistance Vector Table:\n"); 
        for (int i = 0; i < distanceVectorCount; i++) { 
            printf("Destination: %s\n", distanceVector[i].destination); 
            for (int j = 0; j < distanceVector[i].numDistances; j++) { 
                printf(" Via Neighbour: %s, Distance: %d\n", distanceVector[i].distances[j].destination, distanceVector[i].distances[j].distance); 
            } 
        } 
        printf("\n-------------\n"); 
        pthread_mutex_unlock(&distanceVectorMutex); 
        sleep(5); // Print every 5 seconds
    } 
    return NULL;
}

int main() {

    findMyIP();

    pthread_t sendHelloMessage_thread, processIncomingHello_thread, monitorNeighbours_thread, printNeighbourTable_thread, 
    broadcastDistanceVector_thread, processIncomingDV_thread, printDistanceVectorTable_thread;

    pthread_create(&sendHelloMessage_thread, NULL, sendHelloMessage, NULL);
    pthread_create(&processIncomingHello_thread, NULL, processIncomingHello, NULL);
    pthread_create(&monitorNeighbours_thread, NULL, monitorNeighbours, NULL);
    pthread_create(&printNeighbourTable_thread, NULL, printNeighbourTable, NULL);
    pthread_create(&broadcastDistanceVector_thread, NULL, broadcastDistanceVector, NULL);
    pthread_create(&processIncomingDV_thread, NULL, processIncomingDV, NULL);
    pthread_create(&printDistanceVectorTable_thread, NULL, printDistanceVectorTable, NULL);

    pthread_join(sendHelloMessage_thread, NULL);
    pthread_join(processIncomingHello_thread, NULL);
    pthread_join(monitorNeighbours_thread, NULL);
    pthread_join(printNeighbourTable_thread, NULL);
    pthread_join(broadcastDistanceVector_thread, NULL);
    pthread_join(processIncomingDV_thread, NULL);
    pthread_join(printDistanceVectorTable_thread, NULL);

    exit(0);
}
