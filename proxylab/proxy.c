#include <stdio.h>
#include "cache.h"
#include "cache.c"
/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including these long lines in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *accept_hdr = "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
static const char *accept_encoding_hdr = "Accept-Encoding: gzip, deflate\r\n";

/* Function Prototypes */
void *proxy_thread(void *vargp);

/* Global Variables */


int main(int argc, char *argv [])
{
    printf("%s%s%s", user_agent_hdr, accept_hdr, accept_encoding_hdr);

    Signal(SIGPIPE, SIGIGN); // Block SIGPIPE.
	int listenfd, *connfdp, port;
	socklen_t clientlen = sizeof(struct sockaddr_in);
	struct sockaddr_in clientaddr;
	pthread_t tid;

	if (argc != 2) {
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(0);
	}
	port = atoi(argv[1]);

	listenfd = Open_listenfd(port);
	while (1) {
		connfdp = Malloc(sizeof(int));
		*connfdp = Accept(listenfd, (SA *) &clientaddr, &clientlen);
		Pthread_create(&tid, NULL, proxy_thread, connfdp);
	}
	return 0;
}

void *proxy_thread(void *vargp) {
	int fd = *(int *)vargp;
	struct stat sbuf;
	char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
	rio_t rio;

	Pthread_detach(Pthread_self());

	Rio_readinitb(&rio, fd);
	RIO_realineb(&rio, buf, MAXLINE);
	sscanf(buf, "%s %s %s", method, uri, version);

	if (strcasecmp(method, "GET")) {
		clienterror(fd, method, "501", "Not Implemented",
			"Proxy does not support this method");
		return;
	}

	read_requesthdrs(&rio);
}


int parse_uri(char *uri, char *filename, char *cgiargs)
{
	char *ptr;
	if (!strstr(uri, "cgi-bin")) {  /* Static content */
    	strcpy(cgiargs, "");
    	strcpy(filename, ".");
    	strcat(filename, uri);
    	if (uri[strlen(uri)-1] == '/')
      	strcat(filename, "home.html");
   	 	return 1;
  	}
  	else {  /* Dynamic content */
    	ptr = index(uri, '?');
    	if (ptr) {
      		strcpy(cgiargs, ptr+1);
      		*ptr = '\0';
    	}
    	else
      		strcpy(cgiargs, "");
    	strcpy(filename, ".");
    	strcat(filename, uri);
    	return 0;
  	}
}
