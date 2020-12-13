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
#include <unistd.h> 

int main(int argc, char *argv[])
{
	// declare variables
    int socketID = 0, n = 0, port = 0, checkingFileChoice;
    char IP[16];
    char recvBuff[1024];
    char inputStorage[300], FilePath[300], response[300];
    struct sockaddr_in serv_addr; 
	
	// allow for the user to pass in IP and Port
    if(argc != 3)
    {
    	// if they didn't specify, use localhost
        printf("\nUsing LocalHost. To specify connection use: %s <ip of server> <Port Number>\n",argv[0]);
        strcpy(IP,"127.0.0.1");
        port = 8080;
    }else{
    	// else use parameters
    	strcpy(IP,argv[1]);
    	port = atoi(argv[2]);
    }

    memset(recvBuff, '0',sizeof(recvBuff));
    
    // create socket
    if((socketID = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Error : Could not create socket \n");
        return 1;
    } 

    memset(&serv_addr, '0', sizeof(serv_addr)); 
	
	// setup connection on the set port number
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port); 

    if(inet_pton(AF_INET, IP, &serv_addr.sin_addr)<=0)
    {
        printf("\n inet_pton error occured\n");
        return 1;
    } 

	// attempt to connect to the server
    if( connect(socketID, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
       printf("\nConnect could not be established.\n");
       return 1;
    }
    
    // user login variables
    char username[500];
	char password[500];
	
	// prompt for username
	printf("\nUsername: ");
	scanf("%s", username);
	
	checkingFileChoice = 1;
    
    // send username
    if(send(socketID, username, strlen(username), 0) < 0)
	{
		printf("\nError while sending username\n");
		return 1;
	}
	
	// prompt for password
	printf("\nPassword: ");
	scanf("%s", password);
	
	// send password
	if(send(socketID, password, strlen(password), 0) < 0)
	{
		printf("\nError while sending password\n");
		return 1;
	}
	
	// receive message verifying login
	if( (recv(socketID, response, 500, 0) < 0) || (strcmp(response,"Invalid Login")==0))
	{
		// invalid login
		printf("\nServer sent an Error Message:\n");
		printf(response);
		printf("\n");
		return 1;
	}else{
		// valid login
		// while the user hasn't given a valid choice
		while(checkingFileChoice == 1){
			// ask for file path of the file to send
			printf("\nEnter the full path of the file you wish to send:\n");
			scanf("%s", inputStorage);
			
			// if the file can be accessed succesfully, choice is ok
			if(access( inputStorage, F_OK ) == 0 ) {
				checkingFileChoice = 0;	
			}else{
				// else they need to specify a valid file
				printf("\nInvalid File. Please check your spelling.\n You can type 'exit' to leave the program:\n");
			}
			
			// if they asked to exit, exit
			if(strcmp(inputStorage, "exit") == 0){
				return 1;
			}
			
			// store the user file path
			strcpy(FilePath, inputStorage);
		}
		
		// reset menu check
		checkingFileChoice = 1;
		
		// while no valid menu choice
		while(checkingFileChoice == 1){
			// show menu
			printf("\nChoose User Group Folder to Upload to:\n");
			printf("\n*--------------------------------------*\n");
			printf("\n1. Sales\n");
			printf("\n2. Promotions\n");
			printf("\n3. Logistics\n");
			printf("\nYou can also type 'exit' to leave the program:\n");
			// read user menu input
			scanf("%s", inputStorage);
			
			
			// if they asked to exit, exit
			if(strcmp(inputStorage, "exit") == 0){
				return 1;
			}else if((strcmp(inputStorage, "1") == 0) || (strcmp(inputStorage, "2") == 0) || (strcmp(inputStorage, "3") == 0)){
				// valid option
				checkingFileChoice = 0;
			} else {
				// invalid option, check again
				printf("\nInvalid Input. Please Retry:\n");	
			}
		}
		
		
		// send user's menu option		
		if(send(socketID, inputStorage, strlen(inputStorage), 0) < 0)
		{
			printf("Send failed\n");
			return 1;
		}
		
		// receive message about user's access to the folder
		if((recv(socketID, response, 500, 0) < 0) || strcmp(response, "Invalid Folder Access") == 0)
		{
			// unathorized folder
			printf("\nError : \n");
			printf(response);
			printf("\n");
			return 1;
		}else{
			// authorized
			printf(response);
			
			// filename on the server
			printf("\nEnter the filename to save on the server:\n");
			scanf("%s", inputStorage);
			
			// send the file name
			if(send(socketID, inputStorage, strlen(inputStorage), 0) < 0)
			{
				printf("\nSend Filename Failed\n");
				return 1;
			}
			
			char buffer[512];
			char *file_name = FilePath;
			
			printf("\nSending the file %s to the server\n", inputStorage);
			
			// open the file
			FILE *openFile = fopen(FilePath, "r");
			// send the file in blocks
			bzero(buffer, 512);
			int block = 0;
			while((block = fread(buffer, sizeof(char), 512, openFile)) > 0)
			{
				printf("\nSending data\n");
				if(send(socketID, buffer, block, 0) < 0)
				{
					exit(1);
				}
				bzero(buffer, 512);
			}
			
			// check if server received the file successfully
			if(recv(socketID, response, 500, 0) < 0)
			{	
			// error message
				printf("\nError : \n");
				printf(response);
				return 1;
			}else{
				// success message
				printf(response);
				printf("\n\n");
			}
			
		}
	}

    return 0;
}
