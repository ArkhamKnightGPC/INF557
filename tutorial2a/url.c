#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include"url.h"

int convert_to_int(const char *s)
{
	int num=0;
	while(*s){
		int mychar = (*s)-'0';
		if(mychar>=0 && mychar<9){
			num = mychar + num*10;
		}else{
			return -1; //port number is invalid
		}
		s++;   
	}
	return num;
}

int parse_url(char* url, url_info *info)
{
	// url format: <protocol>://<hostname>:<port>/<path> 
	// OBS: port is optional, use 80 as default
	// OBS: protocol is optional, use http as default
	// e.g. https://www.polytechnique.edu:80/index.php

	char mutable_url[203];
	strcpy(mutable_url, url);
	info->protocol = calloc(103, sizeof(char));
	info->host = calloc(103, sizeof(char));
	info->path = calloc(103, sizeof(char));

	char *protocol_separator = strstr(mutable_url, "://");
	if(protocol_separator){
		char aux = *protocol_separator;
		*protocol_separator = '\0';
		strcpy(info->protocol, mutable_url);
		*protocol_separator = aux;

		//we throw an error if protocol is not http
		int cmp = strcmp(info->protocol, "http");
		if(cmp!=0){
			return PARSE_URL_PROTOCOL_UNKNOWN;
		}
	}else{//protocol not specified
		info->protocol = "http";
		protocol_separator = mutable_url-3;
	}

	char *hostname_separator = strstr(protocol_separator+3, "/");
	char *myhost = calloc(103, sizeof(char));
	if(hostname_separator){
		char aux = *hostname_separator;
		*hostname_separator = '\0';
		strcpy(myhost, protocol_separator+3); //this hostname might contain optional port number
		*hostname_separator = aux;
	}else{
		return PARSE_URL_NO_SLASH;
	}

	char *port_separator = strstr(myhost, ":");
	char *myprotocol = calloc(103, sizeof(char));
	if(port_separator){
		char aux = *port_separator;
		*port_separator = '\0';
		strcpy(info->host, protocol_separator+3);
		*port_separator = aux;

		aux = *hostname_separator;
		*hostname_separator = '\0';
		strcpy(myprotocol, port_separator+1);
		*hostname_separator = aux;

		int port_number = convert_to_int(myprotocol);
		if(port_number < 1 || port_number > 65535){
			return PARSE_URL_INVALID_PORT;
		}
		info->port = port_number;
	}else{
		info->port = 80;
		strcpy(info->host, myhost);
	}

	strcpy(info->path, hostname_separator+1);

	// If everything went well, return 0.
	return 0;
}

/**
 * print the url info to std output
 */
void print_url_info(url_info *info){
	printf("The URL contains following information: \n");
	printf("Protocol:\t%s\n", info->protocol);
	printf("Host name:\t%s\n", info->host);
	printf("Port No.:\t%d\n", info->port);
	printf("Path:\t\t/%s\n", info->path);
}

char* get_url_errstr(int err_code){
	return (char*) parse_url_errstr[err_code];
}
