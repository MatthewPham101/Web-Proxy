#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";


//http://icio.us/
//http://sourceware.org/


int main(int argc, char **argv)
{
    //argv means the port defined by the client
    int listenfd, connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    /* Check command line args */
    if (argc != 2) 
	{
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(1);
    }

    listenfd = Open_listenfd(argv[1]);
    while (1) {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); 
        Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, 
                    port, MAXLINE, 0);
        printf("Connection from (%s, %s)\n", hostname, port);
    
    doit(connfd);                                           
    Close(connfd);                                            
    }

    return 0;
}

void doit(int fd) {
    char buf[MAXLINE];
	char method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char hostname[MAXBUF], query[MAXBUF];
    char response[MAXLINE];
	char request[MAXLINE];
    int nServer;
    char cache_data[MAX_OBJECT_SIZE];
    rio_t rio, rio_ser;
	int *port;
    port = malloc(sizeof(int));
    *port = 80;
 
	memset(hostname, 0, sizeof(hostname));
    memset(query, 0, sizeof(query));
    memset(response, 0, sizeof(response));
    memset(cache_data, 0, sizeof(cache_data));
    memset(buf, 0, sizeof(buf));
    memset(request, 0, sizeof(request));
    memset(method, 0, sizeof(method));
    memset(uri, 0, sizeof(uri));
    memset(version, 0, sizeof(method));


    rio_readinitb(&rio, fd);
    if (!rio_readlineb(&rio, buf, MAXLINE))
	{
		return;
	}
    printf("client: %s\n", buf);

    sscanf(buf, "%s %s %s", method, uri, version);
	// if 1.1 then make it 1.0
    if (!strcasecmp(version, "HTTP/1.1")) {
        strcpy(version, "HTTP/1.0");
    }
	// if the request is GET/CONNECT
    if (strcasecmp(method, "GET")) {
        clienterror(fd, method, "501", "error", "Proxy is not compatible");
        return;
    }
    read_requesthdrs(&rio);
	//parse URI
    parseuri(uri, hostname, query, port);
    printf("query: %s\n", query);
    printf("hostname: %s\n", hostname);
    sprintf(request, "%s %s %s\r\n", method, query, version);
    printf("RUI : %s\n", request);

    strcat(request, "Host:");
    strcat(request, hostname);
    strcat(request, "\r\n");
    strcat(request, user_agent_hdr);
    strcat(request, "\r\n");
    strcat(request, "Proxy: close\r\n");
    strcat(request, "Connection: close\r\n");
	
    char port_str[10];
    sprintf(port_str, "%d", *port);
    nServer = open_clientfd(hostname, port_str);
    rio_readinitb(&rio_ser, nServer);


    rio_writen(nServer, request, strlen(request));
    int response_len = rio_readnb(&rio_ser, response, sizeof(response));
    printf("server: %s\n", response);
    rio_writen(fd, response, response_len);
    close(nServer);
}




void read_requesthdrs(rio_t *rp) 
{
    char buf[MAXLINE];
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
    while(strcmp(buf, "\r\n")) {         
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
    }
    return;
}
/// @brief This function will parse the URI 
/// @param uri 
/// @param hostname 
/// @param query 
/// @param port 
void parseuri(const char *uri, char *hostname, char *query, int *port) {
    int storePortNumber[10];
    int i = 0;
	int j = 0;
    for (i = 0; i < 10; i++) 
	{
        storePortNumber[i] = 0;
    }
    
    const char *newURI = uri;
    const char *endURI = newURI + strlen(newURI);
    newURI = strstr(newURI, "://");
    
    if (newURI != NULL) {
        newURI += 3; 
        
        while (newURI < endURI) {
            if (*newURI == ':') {
                newURI++;
                i = 0;
                *port = 0;
                
                while (*newURI != '/' && newURI < endURI) 
				{
                    char port_num[2];
                    port_num[0] = *newURI;
                    port_num[1] = '\0';
                    storePortNumber[i] = atoi(port_num);
                    newURI++;
                    i++;
                }
                
                j = 0;
                while (i > 0)
				{
                    *port = *port + storePortNumber[j] * (i - 1);
                    j++;
                    i--;
                }
            }
            
            if (*newURI != '/') 
			{
                strncat(hostname, newURI, 1);
            } 
			else 
			{
                strcat(hostname, "\0");
                strcpy(query, newURI);
                break;
            }
            
            newURI++;
        }
    }
}

 
 /* $end serve_dynamic */

/*
 * clienterror - returns an error message to the client
 */
/* $begin clienterror */
void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg) 
{
    char buf[MAXLINE];

    /* Print the HTTP response headers */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n\r\n");
    Rio_writen(fd, buf, strlen(buf));

    /* Print the HTTP response body */
    sprintf(buf, "<html><title>Tiny Error</title>");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<body bgcolor=""ffffff"">\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "%s: %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<p>%s: %s\r\n", longmsg, cause);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<hr><em>The Tiny Web server</em>\r\n");
    Rio_writen(fd, buf, strlen(buf));
}
/* $end clienterror */