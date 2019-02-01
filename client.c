/* client.c */

/******************************************************************
 * Remote Shell - Client Program                                  *
 * Author: 	Roberto Ruccia                                        *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define MAX_SLEEP               60
#define BUFFER_SIZE             256
#define DEFAULT_SERVER_PORT		9519
#define DEFAULT_PROTOCOL        0
#define RESULT_BUFFER           32768
#define RCV_TIMEOUT             0
#define RCV_TIMEOUT_U           100000
#define RS_PROMPT_CHAR          "$"
#define LOGIN_CMD               "login"

void error(const char* msg) 
{
	perror(msg);
	exit(1);
}

void bad_input(const char* arg) 
{
	printf("usage: %s -h <host> -p <port> " 
			 "-usr <username> -psw <password>\n", arg);
	exit(0);
}

void instruction() 
{
	printf("+---------------------------+\n");
	//printf("|     Remote Shell - Client   |\n");
	printf("|     sktool         |\n");
	printf("+---------------------------+\n\n");
	//printf("Wait ...\n");
}

int receive_msg(int fd, char* buf) 
{
	int n,total=0;	
	do {
		n = recv(fd,&buf[total],RESULT_BUFFER-total-1,0);
		total += n;
		if(total >= RESULT_BUFFER-1) {
			printf("Buffer overflow.\n");
			exit(1);
		}
	} while(n > 0);
	return total;
}

void authenticate(int fd, char* user, char* psw) 
{
	/* Authentication */	
	char both[BUFFER_SIZE],temp_buf[RESULT_BUFFER];
	sprintf(both, "%s %s:%s\n", LOGIN_CMD, user, psw);
	if(send(fd,both,BUFFER_SIZE,0) < 0) error("ERROR sending message");
	receive_msg(fd,temp_buf);
	printf("%s", temp_buf);		/* Tell if logged or not */
	if(strcmp(temp_buf,"Access denied\n")==0) exit(0);
}

void intHandler(int sig) {
	exit(0);
}




int main(int argc, char* argv[]) 
{
    //signal(SIGINT, intHandler);
	int sockfd, serverLen;
	struct sockaddr_in serverINETAddress;   /* Server Inernet address */
	struct sockaddr* serverSockAddrPtr;     /* Ptr to server address */
	struct hostent* server;                 /* hostent structure */
	
	char command[BUFFER_SIZE], host[100], buf[RESULT_BUFFER];
	char user[100],psw[100];
		
	int port = DEFAULT_SERVER_PORT, result;
    strcpy(host,"127.0.0.1");
	unsigned long inetAddress;  /* 32-bit IP address */
	
	/* Ignore SIGINT and SIGQUIT */
	//signal(SIGINT, SIG_IGN);
	//signal(SIGQUIT, SIG_IGN);	

	//if((argc % 2)==0 || argc > 9) bad_input(argv[0]);
	
	int i;
	for(i = 1; i < argc; i+=2) {
		char* param = argv[i];	
		if(strcmp(param, "-h")==0) strcpy(host, argv[i+1]);
		else if(strcmp(param, "-p")==0) port = atoi(argv[i+1]);
		else if(strcmp(param, "-usr")==0) strcpy(user, argv[i+1]);		
		else if(strcmp(param, "-psw")==0) strcpy(psw, argv[i+1]);	
		else bad_input(argv[0]);	
	}
	
	serverSockAddrPtr = (struct sockaddr*) &serverINETAddress;
	serverLen = sizeof(serverINETAddress);
	bzero((char *) &serverINETAddress, serverLen);
	serverINETAddress.sin_family=AF_INET;
	serverINETAddress.sin_port=htons(port);

	inetAddress = inet_addr(host);      /* get internet address */
	if(inetAddress != INADDR_NONE) {    /* is an IP address */
		serverINETAddress.sin_addr.s_addr = inetAddress;
	} 
	else {      /* is a "hostname" */
		server = gethostbyname(host);
		if(server == NULL) {
			fprintf(stderr, "ERROR, no such host\n");
			exit(0);
		}
		bcopy((char*)server->h_addr,
				(char*)&serverINETAddress.sin_addr.s_addr, server->h_length);
	}	

	sockfd = socket(AF_INET,SOCK_STREAM,DEFAULT_PROTOCOL);  /* create socket */
	if (sockfd < 0) error("ERROR opening socket");
	
	/* Loop until connection is made with the server OR timeout */
	int nsec=1;
	do {
		result = connect(sockfd, serverSockAddrPtr, serverLen);
		if(result == -1) { sleep(1); nsec++; }
	} while(result == -1 && nsec <= MAX_SLEEP);
		
	if(nsec > MAX_SLEEP) error("Connection timeout");
	
	/* Set socket to timeout on recv if no data is available */
	struct timeval tv;
	tv.tv_sec = RCV_TIMEOUT;    /* set timeout */
	tv.tv_usec = RCV_TIMEOUT_U;    /* set timeout */
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,
	(struct timeval*) &tv, sizeof(struct timeval));
	
	instruction();      /* Display instructions */

	//authenticate(sockfd, user, psw);
			
	while(1) {
		printf("%s ", RS_PROMPT_CHAR);
		bzero(command, sizeof(command));	
		fgets(command, BUFFER_SIZE, stdin);
		
		bzero(buf, sizeof(buf));	
		
		/* Send command */
		if(send(sockfd,command,sizeof(command),0) < 0) 
			error("ERROR sending message");

		if (strcmp(command, "exit\n") == 0) break; 

		/* Receive message from server */
		int total = receive_msg(sockfd, buf);

		if(total>0){
			buf[total]='\0';
			printf("%s\n", buf);	
		}
	}
	
	close (sockfd);
					
	return 0;
}	
	
