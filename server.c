#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#define BUFFER_SIZE	1000 	// same as packet size
#define BINDING_TRY_LIMIT	10
int bindRecursive(int socketId, int portNumber, int numberofTry){
	struct sockaddr_in bindaddr;
	int state;
	
	bzero(&bindaddr, sizeof(bindaddr));
	bindaddr.sin_family = AF_INET;
	bindaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	bindaddr.sin_port = htons(portNumber);
	printf("------port number : %d------\n", portNumber);	
	printf("trying to bind...\n");
	state = bind(socketId, (struct sockaddr *)&bindaddr, sizeof(bindaddr));
	if (state == -1) {
		perror("bind error ");
		if (numberofTry < BINDING_TRY_LIMIT) state = bindRecursive(socketId, portNumber + 1, numberofTry +1);
	} else {
		printf("binding succeed\n");
	}
	return state; 
}

int main (int argc, char **argv) {

	int serverSocket;
	int portNumber;
	// Check arguments
	if (argc != 2) {
		printf("Usage: %s <port>\n", argv[0]);
		exit(1);
	} else {
		portNumber = atoi(argv[1]);
	}

	if ( (serverSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
		perror("socket error : ");
		printf("Socket Error\n");
		exit(0);	
	}
	if ( bindRecursive(serverSocket, portNumber, 0) == -1 ){
		printf("failed to bind for %d times, shut down\n", BINDING_TRY_LIMIT);
		exit(0);
	} 
	// TODO: Create sockets for a server and a client
	//      bind, listen, and accept

	// TODO: Read a specified file and send it to a client.
	//      Send as many packets as window size (speified by
	//      client) allows and assume each packet contains
	//      1000 Bytes of data.
	//		When a client receives a packet, it will send back
	//		ACK packet.
	//      When a server receives an ACK packet, it will send
	//      next packet if available.

	// TODO: Print out events during the transmission.
	//		Refer the assignment PPT for the formats.

	// TODO: Close the sockets
	return 0;
}
