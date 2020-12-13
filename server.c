#define _XOPEN_SOURCE
#include <crypt.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <pthread.h>
#include <shadow.h>


// multi thread function for handling connections
void *FileFetchHandler(void *ptr);

// lock file to prevent file clashes
pthread_mutex_t lockFile;

int main(int argc, char *argv[]){
	// declarations
	int socketID, socketClient, port;
	struct sockaddr_in server;
	char sendBuff[1025];
	
	// initialize
	memset(&server, '0', sizeof(server));
    memset(sendBuff, '0', sizeof(sendBuff)); 
	
	// allow the server to user other ports if needed
	if(argc < 2)
    {
		printf("\nUsing Port 8080. To specify connection use: <Port Number>\n");
		port = 8080;
    }else{
    	// set port to user entered value
    	port = atoi(argv[1]);
    }
    
    // notify the user what port is being used
    printf("\nSetting up connection on using port %d \n", port);
	
	
	// setup socket
	if ((socketID = socket(AF_INET, SOCK_STREAM, 0)) == 0){
		perror("Failed to create socket"); 
        exit(EXIT_FAILURE); 	
	}
    
    // setup server properties
    
    // set up server values
	server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(port);
    
    // bind the socket
	if(bind(socketID, (struct sockaddr *)&server, sizeof(server)) < 0){
		perror("Error while binding socket."); 
        	exit(EXIT_FAILURE);
	}
	
	// set a backlog of 3 to allow multiple incoming connections
	listen(socketID, 3);
	
	// loop so the server stays up after each connection is created
	while(1)
	{
		printf("\nThe server is available to connect to:\n");
		
		// wait for connection
		socketClient = accept(socketID, (struct sockaddr*)NULL, NULL);
		
		// check if connection was established
		if(socketClient > -1){
			printf("\nConnection Accepted\n");
			int *client_sock = malloc(200);
			*client_sock = socketClient;
			
			// create a new thread to handle the new connection 		
			pthread_t threadID;
			// create the thread and verify it was created
			if(pthread_create(&threadID, NULL, FileFetchHandler, (void *) client_sock) < 0){
				perror("Error while creating new thread\n");
				// close the socket if the thread could not be established
				close(client_sock);
			}
		}else{
			printf("\nConnection could not be established\n");	
		}
		sleep(1);
	}
}


// this function will run on a seperate thread per connection
void *FileFetchHandler(void *ptr){
	// declarations
	int sock = *(int *) ptr;
	int user_uid, group_uid, user_guid, group_gid;	
	char username[100];
	char password[100];
	char inputHandler[50];
	int i, j;
	gid_t *groups;
	struct passwd *passPerms;
	struct group *gr;
	
	// initialize
	memset(username, 0, 100);
	memset(password, 0, 100);
	memset(inputHandler, 0, 50);
	i = 0;
	j = 10;
	
	
	// retrieve username
	recv(sock, username, 100, 0);
	printf("\nUsername Received :%s \n", username);
	
	// retrieve passsword
	recv(sock, password, 500, 0);

	printf("\nLogin Received. Checking Permissions for % s: \n", username);
	
	// check if the user exists and provided correct password
	if( ((passPerms = getpwnam(username)) == NULL) || (verifyPassword(username, password) != 0) )
	{
		printf("\Invalid login for %s. Killing conneciton.\n", username);
		send(sock, "Invalid Login", strlen("Invalid Login"), 0);
		close(sock);
	}else{

		// get user and group IDs
		user_uid = passPerms->pw_uid;
		group_gid = passPerms->pw_gid;
		
		groups = malloc(j * sizeof(gid_t));
		
		// check if the user is in the correct group
		if(getgrouplist(username, group_gid, groups, &j) > -1)
		{
			// return the account username
			send(sock, username, strlen(username), 0);
			
			printf("\nUser Credentials Accecpted. Waiting on Department choice.\n");
			
			// wait for department choice
			recv(sock, inputHandler, 50, 0);

			char department[20] = "";
			
			// check which directory they are saving to
			if(strcmp(inputHandler, "1") == 0)
			{
				strcpy(department, "sales");
			} else if(strcmp(inputHandler, "2") == 0)
			{
				strcpy(department, "promotions");
			} else if(strcmp(inputHandler, "3") == 0)
			{
				strcpy(department, "logistics");
			};

			// prepare the file path
			char *fileName = department;
			
			for(int i = 0; i < j; i++)
			{
				gr = getgrgid(groups[i]);
				if(gr != NULL)
				{
					if(strcmp(fileName, gr->gr_name) == 0)
					{
						break;
					}
				}
			}
			
			if(strcmp(fileName, gr->gr_name) == 0)
			{
				// notify the client of the successful connection
				send(sock, "Ready To Accept File Path", strlen("Ready To Accept File Path"), 0);
				
				// prepare the receiving path
				char path[500] = "/home/ben/Documents/FileShare/Storage/";
				strcat(path, fileName);
				strcat(path, "/");
				
				// clear input
				memset(inputHandler, 0 , 50);
				
				// receive the user input
				recv(sock, inputHandler, 50, 0);
				
				// add file name to path
				//strcat(path, department);
				strcat(path, inputHandler);

				char buffer[512];

				// print the end path
				strcpy(fileName, path);
				printf("\nFile to be transferred to: %s\n", fileName);

				//lock file for synchronisation
				pthread_mutex_lock(&lockFile);

				// open/create the destination file
				FILE *newFile = fopen(fileName, "w");

				if(newFile == NULL)
				{
					printf("\n %s could not be opened/created by the server\n\n", fileName);
					close(sock);
				} else  {
					bzero(buffer, 512);
					int data = 0;
					//transfer the file data
					while((data = recv(sock, buffer, 512, 0)) > 0)
					{
						printf("\nReceived data\n");
						int fileSize = fwrite(buffer, sizeof(char), data, newFile);
						bzero(buffer, 512);
						if(fileSize == 0 || fileSize != 512)
						{
							break;
						}
					}
					printf("\nThe file has be received successfully.\n\n");
					fclose(newFile);
					
					//unlock the file
					pthread_mutex_unlock(&lockFile);

					
					chown(fileName, user_uid, gr->gr_gid);
					
					printf("\n");
					send(sock, "File transfer complete", strlen("File transfer complete"), 0);

					//log the user, date and file details
					char logFile[200] = "/home/ben/Documents/FileShare/Logs/ServerLogs.txt";
					char log[1000];
					char date[30];
					
					// get current time
					time_t now = time(0);
					struct tm *timeDate;
					timeDate = gmtime(&now);
					// format time
					strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", timeDate);
					// add username
					strcat(log, "Username: ");
					strcat(log, username);
					// add department
					strcat(log, "\nDepartment: ");
					strcat(log, department);
					// list file name
					strcat(log, "\nFile: ");
					strcat(log, path);
					strcat(log, "\nTimestamp: ");
					strcat(log, date);
					strcat(log, "\n\n");

					FILE *logfile = fopen(logFile, "a");
					if(logfile != NULL)
					{
						fputs(log, logfile);
						fclose(logfile);
					}
					else
					{
						printf("\nError while editing log : %s. Please ensure Log folder is present.\n", logFile);
					}		
				}
				
			}else{
				printf("\nThe file could not be accessed\n\n");
				send(sock, "Invalid Folder Access", strlen("Invalid Folder Access"), 0);
				close(sock);
			}
		}
	}
}

int verifyPassword(const char* username, const char* password){
	struct passwd* pass = getpwnam(username);
    if(!pass)
    {
        printf( "\nInvalid User : %s\n", username);
        return 1;
    }

    if (strcmp( pass->pw_passwd, "x" ) != 0)
    {
    	// return 0 if correct, otherwise incorrect password
        return strcmp( pass->pw_passwd, crypt( password, pass->pw_passwd ) );
    }
    else
    {
        struct spwd* spw = getspnam(username);
        if ( !spw )
        {
            printf( "Could not verify password '%s'\n",username);
            // return incorrect password
            return 1;
        }
		// return 0 if correct, otherwise incorrect password
        return strcmp( spw->sp_pwdp, crypt( password, spw->sp_pwdp ) );
    }
}

