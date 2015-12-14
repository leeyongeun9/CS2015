#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include "constants.h"

int clientSocket;

int main (int argc, char **argv) {
	struct sockaddr_in clientaddr;
	int socketLen;

	int portNumber, i;
	char *windowSize, *delay;
	char *hostName;
	char bufIn[16];
	FILE *fp;
	
	clientSocket = socket(AF_INET, SOCK_STREAM, 0);



	// Check arguments 
	if (argc != 7) {
		printf("Usage: %s <hostname> <port> [Arguments]\n", argv[0]);
		printf("\tMandatory Arguments:\n");
		printf("\t\t-w\t\tsize of the window used at server\n");
		printf("\t\t-d\t\tACK delay in msec");
		exit(1);
	}
	hostName = argv[1];
	portNumber = atoi(argv[2]);
	for (i = 3 ; i < argc ; i++ ) {
		if (strncmp(argv[i], "-w", 2) == 0){
			windowSize = argv[i+1];
		}
		if (strncmp(argv[i], "-d", 2) == 0) {
			delay = argv[i+1];
		}
	}
	printf("windowSize is : %d, delay is : %d\n", atoi(windowSize), atoi(delay));
	printf("\n");
	printf("List of inputs\n");
	printf("\tC : Connect to the server\n");
	printf("\tR n : Request to server to transmit file number n(n = 1, 2, 3)\n");
	printf("\tF: Finish the connection to the server\n");
	printf("\n");

	clientaddr.sin_family = AF_INET;
	clientaddr.sin_addr.s_addr = inet_addr(hostName);
	clientaddr.sin_port = htons(portNumber);

	socketLen = sizeof(clientaddr);   
	while(1){
		printf("Enter your command : ");
		memset(bufIn, '0', 16);
		fgets(bufIn, 16, stdin);
		bufIn[strlen(bufIn)-1] = '\0';
		if (bufIn[0] == 'C'){
			if( connect(clientSocket, (struct sockaddr *)&clientaddr, socketLen) ) {
				printf("failed to connect for %d times, shut down\n", CONNECT_TRY_LIMIT);
				exit(1);
			}
			char *buf = malloc(strlen(windowSize)+2);
			char *step = buf;	
			step[0] = 'C';
			step ++;
			memcpy(step, windowSize, strlen(windowSize) + 1);
			send(clientSocket, buf, strlen(buf), 0);
			free(buf);

		} else if (bufIn[0] == 'R') {
			int sending = 0;
			for ( i = 1 ; i < strlen(bufIn) ; i ++ ) {
				if ( bufIn[i] >= '1' && bufIn[i] <= '3' ) {
					char message[20];
					  int j;
					  for (j = 0 ; j < 20 ; j++){
					    message[j] = '1';
					  }
					message[19] = '\0';
					sending = send(clientSocket, message, strlen(message), 0);
				}
			}
			if (sending != 2) printf("n can be only 1, 2 or 3\n"); 
			
		} else if (bufIn[0] == 'F') {
			send(clientSocket, quitStr, strlen(quitStr), 0);
			break;
		} else {
			printf("\tundefined input : %s\n", bufIn);
			printf("\tList of inputs\n");
			printf("\tC : Connect to the server\n");
			printf("\tR n : Request to server to transmit file number n(n = 1, 2, 3)\n");
			printf("\tF: Finish the connection to the server\n");
		}
			
	}	
	return 0;
}
