/* server.c */

/******************************************************************
 * Remote Shell - Server Program                                  *
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

#define BUFFER_SIZE                     256
#define QUEUE_LENGTH                    5
#define DEFAULT_SERVER_PORT             5000
#define LOGIN_CMD                       "login"
#define FALSE                           0
#define TRUE                            1

void error(const char* msg) 
{
	perror(msg);
	exit(1);    /* Abort */
}

void bad_input(const char* arg) 
{
	printf("usage: %s -p <port> -c </full/path/to/commands_file> "
			 "-u </full/path/to/users_file>\n", arg);
	exit(0);
}

void instruction(int p) 
{
	printf("+-------------------------------+\n");
	printf("|     Remote Shell - Server     |\n");
	printf("+-------------------------------+\n");
	printf("| Server listening on port %d |\n", p);
	printf("|     Ctrl+C to end process     |\n");
	printf("+-------------------------------+\n\n");
}

int parse_file(const char* path_to_file, char* str) 
{
	FILE* fp = fopen(path_to_file, "r");
	char line[BUFFER_SIZE];
	int i;
	if(fp != NULL) {
		while(fgets(line, sizeof(line), fp) != NULL) {
			i = 0;
			while(line[i] == ' ') i++;          /* Skip leading spaces */
			if(line[i] == '#') continue;        /* Skip lines beginning with '#' */
			if(strcmp(&line[i],str)==0) {
				fclose(fp);
				return TRUE;    /* File contains str */
			}
		} 
	}
	else { fclose(fp); error ("ERROR opening file"); }
	fclose(fp);
	return FALSE;   /* File does not contain str */
}

int file_exists(const char* path_to_file) 
{
	FILE* fp;
	if(path_to_file!= NULL) {
		if((fp = fopen(path_to_file,"r"))==NULL) 
			fclose(fp);
		else {
			printf("File '%s' does not exist\n",path_to_file);
			return FALSE;   /* Not exists */
		}
	}
	return TRUE;
}

int is_allowed_cmd(const char* path_to_file, char* str) 
{
	char command[BUFFER_SIZE];
	sprintf(command, "%s", str);
	int len = strlen(command);
	if(command[len-1] != '\n') {/* command must end with newline */
		command[len] = '\n';	command[len+1] = '\0';		
	}
	if(!parse_file(path_to_file, command)) 
		return FALSE;       /* Not runnable */

	return TRUE;    /* Is runnable */		
}

int main(int argc, char* argv[]) 
{
	int sockfd, newsockfd, serverLen, clientLen;		
	int port = DEFAULT_SERVER_PORT;                     /* port number */
	char buffer[BUFFER_SIZE], temp_buf[BUFFER_SIZE];    /* Buffers */
	char* cmd_file_pathname = NULL;                     /* Commands filename */
	char* usr_file_pathname = NULL;                     /* Users filename */
	struct sockaddr_in serverINETAddress;               /* Server Internet address */
	struct sockaddr* serverSockAddrPtr;	                /* Ptr to server address */
	struct sockaddr_in clientINETAddress;               /* Client Internet address */
	struct sockaddr* clientSockAddrPtr;                 /* Ptr to client address */
		
	/* Ignore death-of-child signals to prevent zombies */
	signal (SIGCHLD, SIG_IGN);
	
	if((argc % 2)==0||argc > 7) bad_input(argv[0]);
	
	int i;	
	for(i = 1; i < argc; i+=2) {    /* Parse command line */
		char* param = argv[i];
		
		if(strcmp(param, "-p")==0) port = atoi(argv[i+1]);
		else if(strcmp(param, "-c")==0) {
			cmd_file_pathname= malloc(BUFFER_SIZE);
			strcat(cmd_file_pathname,"/");
			strcat(cmd_file_pathname,argv[i+1]);
		}
		else if(strcmp(param, "-u")==0) {
			usr_file_pathname= malloc(BUFFER_SIZE);
			strcat(usr_file_pathname,"/");
			strcat(usr_file_pathname,argv[i+1]);
		}
		else bad_input(argv[0]);
	}
	
	if(!file_exists(cmd_file_pathname) || !file_exists(usr_file_pathname)) 
		return 0;

	/* Create the server Internet socket */
	sockfd = socket(AF_INET, SOCK_STREAM, /* DEFAULT_PROTOCOL */ 0);	
	if(sockfd < 0) error("ERROR opening socket");
	
	/* Fill in server socket address fields */
	serverLen = sizeof(serverINETAddress);
	bzero((char*)&serverINETAddress, serverLen);            /* Clear structure */
	serverINETAddress.sin_family = AF_INET;                 /* Internet domain */
	serverINETAddress.sin_addr.s_addr = htonl(INADDR_ANY);  /* Accept all */
	serverINETAddress.sin_port = htons(port);               /* Server port number */	
	serverSockAddrPtr = (struct sockaddr*) &serverINETAddress;
	
	/* Bind to socket address */
	if(bind(sockfd, serverSockAddrPtr, serverLen) < 0) 
		error("ERROR on binding");
	
	/* Set max pending connection queue length */
	if(listen(sockfd, QUEUE_LENGTH) < 0) error("ERROR on listening");
	
	clientSockAddrPtr = (struct sockaddr*) &clientINETAddress;
	clientLen = sizeof(clientINETAddress);
	
	instruction(port);  /* Display instructions */
	
	while(1) {  /* Loop forever */
		/* Accept a client connection */
		newsockfd = accept(sockfd,/*NULL*/clientSockAddrPtr,/*NULL */&clientLen);		
		if(newsockfd < 0) error("ERROR on accept");
		
		printf("Server %d: connection %d accepted\n", getpid(), newsockfd);
		
		int logged = FALSE; /* not logged in */
		
		if(fork() == 0) {   /* Create a child to serve client */
			/* Perform redirection */
			int a = dup2(newsockfd, STDOUT_FILENO);
			int b = dup2(newsockfd, STDERR_FILENO);
			if(a < 0 || b < 0) error("ERROR on dup2");
			
			while(1) {				
				bzero(buffer, sizeof(buffer));
				
				/* Receive */
				if(recv(newsockfd, buffer, sizeof(buffer), 0) < 0) 
					error("ERROR on receiving");
			
				/* Log in, if necessary */
				if( !logged && usr_file_pathname != NULL) {
					if(!parse_file(usr_file_pathname,&buffer[sizeof(LOGIN_CMD)])) {
						printf("Access denied\n"); 
						close(newsockfd);
						exit(0);
					}
					else { printf("Logged in\n"); logged=TRUE; continue; }
				}
				
				if (strcmp(buffer, "exit\n") == 0) {    /* Exit */
					close(newsockfd);
					exit(0);
				}
				
				/* Tokenize command */
				bzero(temp_buf, sizeof(temp_buf));				
				strcpy(temp_buf, buffer);				
				char* p = strtok(temp_buf, " ");    /* token (cmd) */
				
				if(strcmp(p, LOGIN_CMD)==0) continue;   /* Ignore LOGIN_CMD */
			
				/* Check if client can run this command */			
				if(cmd_file_pathname != NULL && 
					!is_allowed_cmd(cmd_file_pathname,p)) {
					printf("Command not allowed\n");
					continue;
				}
				
				/* Manage 'cd' */
				if(strcmp(p, "cd")== 0 ) {
					p = strtok(NULL, " ");  /* This is the new path */			
					p[strlen(p)-1] = '\0';  /* replace newline termination char */
					
					/* Change current working directory */					
					if(chdir(p) < 0) perror("error");				
				} 
				else system(buffer);    /* Issue a command */			
			}
		}
		else close (newsockfd); /* Close newsocket descriptor */
	}
	
	close(sockfd);
	
	return 0;   /* Terminate */
}

