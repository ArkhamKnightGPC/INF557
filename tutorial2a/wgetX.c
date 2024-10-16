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
#include <unistd.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>
#include <regex.h>

#include "url.h"
#include "wgetX.h"

int main(int argc, char* argv[]) {
    url_info info;
    const char * file_name = "received_page";
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
    //print_url_info(&info);

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
    //https://csperkins.org/teaching/2009-2010/networked-systems/lab04.pdf
    
    addrinfo *host = (addrinfo *)calloc(1, sizeof(addrinfo));
    sockaddr_in *host_addr = (sockaddr_in *)calloc(1, sizeof(sockaddr_in));

    if(getaddrinfo(info->host, NULL, NULL, &host) != 0){
        fprintf(stderr, "Could not find host address\n");
        return 1;
    }
    host_addr = (sockaddr_in *)host->ai_addr;
    host_addr->sin_port = htons(80);

    //char *host_ip = (char *)inet_ntoa(host_addr->sin_addr);
    //printf("FAMILY %d  SOCKTYPE %d PROTOCOL %d\n", 
    //    host->ai_family, host->ai_socktype, host->ai_protocol);
    //printf("debug %s\n", host_ip);
    //printf("FAMILY %d PORT %d\n", host_addr->sin_family, host_addr->sin_port);
    //printf("FAMILY %d PORT %d PROTOCOL 0\n", AF_INET, info->port);

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
    //https://cboard.cprogramming.com/c-programming/142841-sending-http-get-request-c.html

    //int client_socket = socket(host->ai_family, host->ai_socktype, host->ai_protocol);
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(client_socket < 0){
        fprintf(stderr, "Could not open socket\n");
        return 1;
    }
    socklen_t slen = sizeof(sockaddr);

    //printf("BEFORE CONNECT %d\n", client_socket);
    int connfd = connect(client_socket, (sockaddr *)host_addr, slen);
    if(connfd < 0){
        fprintf(stderr, "Could not connect\n");
        return 1;
    }
    //printf("AFTER CONNECT %d\n", connfd);

    char *sendBuff = http_get_request(info);
    if( write(client_socket, sendBuff, strlen(sendBuff)) < 0){
        fprintf(stderr, "Could not send request\n");
        return 1;
    }

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
    const int my_buffer_size = 1024;
    reply->reply_buffer = (char *)calloc(my_buffer_size, sizeof(char));
    reply->reply_buffer_length = my_buffer_size;

    char *tmp_buffer = (char *)calloc(my_buffer_size, sizeof(char));

    int keep_receiving = 1, len = 0;
    do{
        //int bytes_received = recv(client_socket, reply->reply_buffer, reply->reply_buffer_length - len, 0);
        int bytes_received = recv(client_socket, tmp_buffer, my_buffer_size, 0);
        len += bytes_received;
        if(len >= reply->reply_buffer_length){
            reply->reply_buffer_length = len + 1;
            reply->reply_buffer = realloc(reply->reply_buffer, reply->reply_buffer_length);
        }
        if(bytes_received <= 0){
            keep_receiving = 0;
        }else{
            memcpy(reply->reply_buffer + len - bytes_received, tmp_buffer, bytes_received);   
        }
    }while(keep_receiving);

    //printf("%s\n", reply->reply_buffer);
    reply->reply_buffer_length = len;
    //printf("%d\n", reply->reply_buffer_length);

    close(client_socket);
    free(host);
    free(sendBuff);

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
    if( fwrite(data, 1, len, fp) < 0){
        fprintf(stderr, "Could not write to file\n");
        return;
    }
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

void find_new_url(char *redirect_reply, char *new_url) {//ok I need to find string in redirect href=""
    regex_t regex;
    regmatch_t match[2];

    const char *regex_pattern = "href=\"([^\"]*)\"";
    int reti = regcomp(&regex, regex_pattern, REG_EXTENDED);

    if (reti) {
        fprintf(stderr, "For some reason regcomp failed :(\n");
        return;
    }

    reti = regexec(&regex, redirect_reply, 2, match, REG_EXTENDED);

    if (!reti) {
        int start = match[1].rm_so;
        int end = match[1].rm_eo;
        int length = end - start;

        if (new_url == NULL) {
            fprintf(stderr, "Memory allocation failed\n");
            regfree(&regex);
            return;
        }

        strncpy(new_url, redirect_reply + start, length);
        new_url[length] = '\0';

        regfree(&regex);
    } else if (reti == REG_NOMATCH) {
        printf("No match found\n");
    } else {
        char error_message[100];
        regerror(reti, &regex, error_message, sizeof(error_message));
        fprintf(stderr, "Regex match failed: %s\n", error_message);
    }

    return;
}

char *read_http_reply(struct http_reply *reply) {

    struct http_reply *reply_copy = (struct http_reply *)malloc(sizeof(struct http_reply));
    reply_copy->reply_buffer = (char *)calloc(1, reply->reply_buffer_length*sizeof(char));
    strcpy(reply_copy->reply_buffer, reply->reply_buffer);
    reply_copy->reply_buffer_length = reply->reply_buffer_length;

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

    //printf("%s\n", reply_copy->reply_buffer);

    if (rv != 2) {
        fprintf(stderr, "Could not parse http response first line (rv=%d, %s)\n", rv, reply->reply_buffer);
        return NULL;
    }

    if(status == 301 || status == 302){
        //fprintf(stderr, "Redirect\n");
        //printf("%s\n", reply_copy->reply_buffer);

        struct http_reply *new_reply = malloc(sizeof(struct http_reply));
        char *new_url = (char *)calloc(1, sizeof(char));
        find_new_url(reply_copy->reply_buffer, new_url);
        url_info *new_urlinfo = (url_info *)calloc(1, sizeof(url_info));

        if(parse_url(new_url, new_urlinfo) != 0){
            fprintf(stderr, "Problem with redirect URL\n");
            return NULL;
        }

        //print_url_info(new_urlinfo);

        if(download_page(new_urlinfo, new_reply)){
            fprintf(stderr, "Problem downloading redirect page\n");
            return NULL;
        }

        return read_http_reply(new_reply);
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
     *   Difficul challenge:
     *     If you feel like having a real challenge, go on and implement HTTP redirect support for your client.
     *
     */
    int reply_length = reply->reply_buffer_length - sizeof(reply->reply_buffer) - 2;

    while(*buf != '\r'){

        status_line = next_line(buf, reply_length);
        *status_line = '\n';

        reply_length -= sizeof(buf) + 2;
        buf = status_line + 2;
    }   

    buf += 2;

    //printf("%s\n", buf);

    return buf;
}
