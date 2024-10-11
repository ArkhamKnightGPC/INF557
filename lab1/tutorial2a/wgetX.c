/**
 *  Jiazi Yi
 * LIX, Ecole Polytechnique
 * jiazi.yi@polytechnique.edu
 *
 * Updated by Pierre Pfister
 * Cisco Systems
 * ppfister@cisco.com
 *
 * Updated by Kevin Jiokeng
 * LIX, Ecole Polytechnique
 * kevin.jiokeng@polytechnique.edu
 *
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "url.h"
#include "wgetX.h"

int main(int argc, char* argv[]) {

    url_info info;
    const char *file_name = "received_page";

    if (argc < 2) {
        fprintf(stderr, "Missing argument. Please enter URL.\n");
        return 1;
    }

    char *url = argv[1];

    // Get optional file name
    if (argc > 2) {
	    file_name = argv[2];
    }

    // First parse the URL
    int ret = parse_url(url, &info);
    if (ret) {
	    fprintf(stderr, "Could not parse URL '%s': %s\n", url, parse_url_errstr[ret]);
	    return 2;
    }

    //If needed for debug
    print_url_info(&info);

    // Download the page
    struct http_reply reply;

    ret = download_page(&info, &reply);
    if (ret) {
	    return 3;
    }

    // Now parse the responses
    char *response = read_http_reply(&reply);
    if (response == NULL) {
        fprintf(stderr, "Could not parse http reply\n");
        return 4;
    }

    // Write response to a file
    write_data(file_name, response, reply.reply_buffer + reply.reply_buffer_length - response);

    // Free allocated memory
    free(reply.reply_buffer);

    // Just tell the user where is the file
    fprintf(stderr, "the file is saved in %s.", file_name);
    return 0;
}

int download_page(url_info *info, http_reply *reply) {

    /*
     * To be completed:
     *   You will first need to resolve the hostname into an IP address.
     *
     *   Option 1: Simplistic
     *     Use gethostbyname function.
     *
     *   Option 2: Challenge
     *     Use getaddrinfo and implement a function that works for both IPv4 and IPv6.
     *
     */
    hostent *host_ip = gethostbyname(info->host);
    if(!host_ip){
        fprintf(stderr, "error getting host ip address\n");
        return 1;
    }
    //source: https://www.geeksforgeeks.org/c-program-display-hostname-ip-address/
    char *ip_address = inet_ntoa(*((struct in_addr*)host_ip->h_addr_list[0]));
    printf("%s\n", ip_address);

    /*
     * To be completed:
     *   Next, you will need to send the HTTP request.
     *   Use the http_get_request function given to you below.
     *   It uses malloc to allocate memory, and snprintf to format the request as a string.
     *
     *   Use 'write' function to send the request into the socket.
     *
     *   Note: You do not need to send the end-of-string \0 character.
     *   Note2: It is good practice to test if the function returned an error or not.
     *   Note3: Call the shutdown function with SHUT_WR flag after sending the request
     *          to inform the server you have nothing left to send.
     *   Note4: Free the request buffer returned by http_get_request by calling the 'free' function.
     *
     */
    char sendBuff[1025];
    char *request_buffer = http_get_request(info);
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if(listenfd < 0){
        fprintf(stderr, "error opening socket\n");
        return 1;
    }

    sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    memset(sendBuff, 0, sizeof(sendBuff));
    //source: https://cboard.cprogramming.com/c-programming/142841-sending-http-get-request-c.html
    serv_addr.sin_family = AF_INET;
    bcopy(ip_address, (char *)&serv_addr.sin_addr.s_addr, strlen(ip_address));
    //serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(info->port);

    int conn_status = connect(listenfd, (sockaddr*)&serv_addr, sizeof(serv_addr));
    if(conn_status == -1){
        fprintf(stderr, "connection failed\n");
        return 1;
    }
    if(write(listenfd, request_buffer, strlen(request_buffer)) < 0){
        fprintf(stderr, "write failed\n");
        return 1;
    }
    close(listenfd);
    //shutdown(listenfd, SHUT_WR);
    //free(request_buffer);

    /*
     * To be completed:
     *   Now you will need to read the response from the server.
     *   The response must be stored in a buffer allocated with malloc, and its address must be save in reply->reply_buffer.
     *   The length of the reply (not the length of the buffer), must be saved in reply->reply_buffer_length.
     *
     *   Important: calling recv only once might only give you a fragment of the response.
     *              in order to support large file transfers, you have to keep calling 'recv' until it returns 0.
     *
     *   Option 1: Simplistic
     *     Only call recv once and give up on receiving large files.
     *     BUT: Your program must still be able to store the beginning of the file and
     *          display an error message stating the response was truncated, if it was.
     *
     *   Option 2: Challenge
     *     Do it the proper way by calling recv multiple times.
     *     Whenever the allocated reply->reply_buffer is not large enough, use realloc to increase its size:
     *        reply->reply_buffer = realloc(reply->reply_buffer, new_size);
     *
     *
     */
    int len, mysocket;
    sockaddr_in dest;

    mysocket = socket(AF_INET, SOCK_STREAM, 0);
    memset(&dest, 0, sizeof(dest));

    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    dest.sin_port = htons(info->port);

    connect(mysocket, (sockaddr*)&dest, sizeof(sockaddr));
    char *complete_reply, *ith_reply;
    complete_reply = calloc(1025, sizeof(char));
    ith_reply = calloc(1025, sizeof(char));
    int still_receiving = 1;
    do{
        int bytes_received = recv(mysocket, ith_reply, sizeof(ith_reply), 0);
        if(bytes_received < 0){
            still_receiving = 0;
        }else{
            len += bytes_received;
            if(len > strlen(complete_reply)){
                int new_size = 2*strlen(complete_reply);
                complete_reply = realloc(complete_reply, new_size);
            }
        }
    }while(still_receiving);
    
    reply->reply_buffer_length = len;
    strcpy(reply->reply_buffer, complete_reply);

    return 0;
}

void write_data(const char *path, const char * data, int len) {
    /*
     * To be completed:
     *   Use fopen, fwrite and fclose functions.
     */
    FILE *fp = fopen(path, "w");
    if(!fp){
        fprintf(stderr, "Could not find file\n");
        return;
    }
    fprintf(fp, "%s", data);
    fclose(fp);
}

char* http_get_request(url_info *info) {
    char * request_buffer = (char *) malloc(100 + strlen(info->path) + strlen(info->host));
    snprintf(request_buffer, 1024, "GET /%s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n",
	    info->path, info->host);
    return request_buffer;
}

char *next_line(char *buff, int len) {
    if (len == 0) {
	    return NULL;
    }

    char *last = buff + len - 1;
    while (buff != last) {
	if (*buff == '\r' && *(buff+1) == '\n') {
	    return buff;
	}
	buff++;
    }
    return NULL;
}

char *read_http_reply(struct http_reply *reply) {

    // Let's first isolate the first line of the reply
    char *status_line = next_line(reply->reply_buffer, reply->reply_buffer_length);
    if (status_line == NULL) {
	    fprintf(stderr, "Could not find status\n");
	    return NULL;
    }
    *status_line = '\0'; // Make the first line is a null-terminated string

    // Now let's read the status (parsing the first line)
    int status;
    double http_version;
    int rv = sscanf(reply->reply_buffer, "HTTP/%lf %d", &http_version, &status);
    if (rv != 2) {
        fprintf(stderr, "Could not parse http response first line (rv=%d, %s)\n", rv, reply->reply_buffer);
        return NULL;
    }

    if (status != 200) {
        fprintf(stderr, "Server returned status %d (should be 200)\n", status);
        return NULL;
    }

    char *buf = status_line + 2;

    /*
     * To be completed:
     *   The previous code only detects and parses the first line of the reply.
     *   But servers typically send additional header lines:
     *     Date: Mon, 05 Aug 2019 12:54:36 GMT<CR><LF>
     *     Content-type: text/css<CR><LF>
     *     Content-Length: 684<CR><LF>
     *     Last-Modified: Mon, 03 Jun 2019 22:46:31 GMT<CR><LF>
     *     <CR><LF>
     *
     *   Keep calling next_line until you read an empty line, and return only what remains (without the empty line).
     *   Hint: Take a look at how end of lines are tested in next_line function declaration, to get inspiration
     *
     *   Difficult challenge:
     *     If you feel like having a real challenge, go on and implement HTTP redirect support for your client.
     *
     */

    int keep_calling = 1;
    do{
        char *myline = next_line(buf, strlen(buf));
        if(!myline){
            keep_calling = 0;
        }else{

        }
    }while(keep_calling);

    return buf;
}
