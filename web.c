#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
//#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define VERSION 23
#define BUFSIZE 8096

void logger(char *s1, char *s2)        
{
	int fd;
	char logbuffer[BUFSIZE*2];
	
	if((fd = open("nweb.log", O_CREAT| O_WRONLY | O_APPEND,0644)) >= 0) {
		sprintf(logbuffer, "s1: %s\ns2: %s\n", s1, s2);
		(void)write(fd,logbuffer,strlen(logbuffer)); 
		(void)write(fd,"\n",1);      
		(void)close(fd);
	}
}

/* this is a child web server process, so we can exit on errors */
void web(int fd)
{
	int j, file_fd, buflen;
	long i, ret, len;
	char * fstr;
	static char buffer[BUFSIZE+1]; /* static so zero filled */

	ret =read(fd,buffer,BUFSIZE); 	/* read Web request in one go */
	if(ret == 0 || ret == -1) {	/* read failure stop now */
		exit(3);
	}
	if(ret > 0 && ret < BUFSIZE)	/* return code is valid chars */
		buffer[ret]=0;		/* terminate the buffer */
	else buffer[0]=0;
	
	(void)sprintf(buffer,"HTTP/1.1 200 OK\nServer: nweb_yunia_modification\nContent-Length: %ld\nConnection: close\nContent-Type: text/html\n\n", 0); /* Header + a blank line */
	(void)write(fd,buffer,strlen(buffer));

	sleep(1);	/* allow socket to drain before signalling the socket is closed */
	close(fd);
	exit(1);
}

int main(int argc, char **argv)
{
	int i, port, pid, listenfd, socketfd;
	socklen_t length;
	static struct sockaddr_in cli_addr; /* static = initialised to zeros */
	static struct sockaddr_in serv_addr; /* static = initialised to zeros */

	if( argc < 3  || argc > 3 || !strcmp(argv[1], "-?") ) {
		(void)printf("hint: nweb Port-Number Top-Directory\t\tversion %d\n\n"
	"\tnweb is a small and very safe mini web server\n"
	"\tnweb only servers out file/web pages with extensions named below\n"
	"\t and only from the named directory or its sub-directories.\n"
	"\tThere is no fancy features = safe and secure.\n\n"
	"\tExample: nweb 8181 /home/nwebdir &\n\n"
	"\tOnly Supports:", VERSION);
	}

	if(chdir(argv[2]) == -1){ 
		(void)printf("ERROR: Can't Change to directory %s\n",argv[2]);
		exit(4);
	}
	
	(void)signal(SIGCLD, SIG_IGN); /* ignore child death */
	for(i=0;i<32;i++)
		(void)close(i);		/* close open files */
	
	/* setup the network socket */
	if((listenfd = socket(AF_INET, SOCK_STREAM,0)) <0)
		logger("system call","socket");
	port = atoi(argv[1]);
	if(port < 0 || port >60000)
		logger("Invalid port number (try 1->60000)",argv[1]);
	
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(port);
	if(bind(listenfd, (struct sockaddr *)&serv_addr,sizeof(serv_addr)) <0)
		logger("system call","bind");
	
	if( listen(listenfd,64) <0)
		logger("system call","listen");
	
	for(;; ) {
		length = sizeof(cli_addr);
		if((socketfd = accept(listenfd, (struct sockaddr *)&cli_addr, &length)) < 0)
			logger("system call","accept");
		if((pid = fork()) < 0) {
			logger("system call","fork");
		}
		else {
			if(pid == 0) { 	/* child */
				(void)close(listenfd);
				web(socketfd); /* never returns */
			} else { 	/* parent */
				(void)close(socketfd);
			}
		}
	}
}
