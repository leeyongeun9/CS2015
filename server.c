#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "constants.h"
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
void sendFile(int socket, FILE *fl, int windowSize) {
	myPacketBuffer *buf;
	char buffer[BUFFER_SIZE+HEADER_SIZE];
	char *pt;
	ackPacket *ackbuf;
	char ackbuffer[10];
	int minSN = 0, maxSN = -1;
	int diffSN = 0;
	unsigned long i = 0;
	unsigned long j = 0;
	
	pt = buf + HEADER_SIZE;	
	while(fgets(buf->pack, BUFFER_SIZE, fl) != '\0') {
		if (i > 40) break;
		i ++;
		pt = buf;
		if (minSN + windowSize -1 > maxSN) {
			buf->sn = maxSN;
			buf->length = strlen(buf->pack);
			int count = send(socket, (void *)buf, sizeof(buf), 0);
			maxSN++;
		} else {
			void *temp;
			int count = recv(socket, temp, sizeof(ackPacket), 0);
			ackbuf = (ackPacket *) temp;
			if ( strncmp(ackbuffer, ackbuf->msg, strlen(ackbuf->msg)) == 0 ) {
				minSN = ackbuf->rn;
			}
		}
		/* 
		if (diffSN < windowSize){
			//printf("the first character is : %c\n", buffer[0]);
			diffSN ++;
			int count = send(socket, buffer, sizeof(buffer), 0);	
			j++;
			printf("j : %d\n", j);
		} else {
			int count = recv(socket, ackbuffer, strlen(ACK), 0);
			if ( count > 0 ) ackbuffer[count] = '\0';
			if ( strncmp(ackbuffer, ACK, strlen(ackbuffer)) == 0 ) {
				diffSN --;
			//	printf("ACK : %s\n", ACK);
			}
		}*/
	} 
	buf->sn++;
	buf->length = strlen(transferFinished);
	strncpy(buf->pack, transferFinished, buf->length); 
	send(socket, buf, sizeof(buf), 0);
}

int main (int argc, char **argv) {

	int serverSocket, clientSocket;
	int portNumber;
	int count;
	int windowSize;
	socklen_t clientLen;
	struct sockaddr_in clientaddr;

	char buf[255];
	FILE *fl;
	clientLen = sizeof(clientaddr);	
	// Check arguments
	if (argc != 2) {
		printf("Usage: %s <port>\n", argv[0]);
		exit(1);
	} else {
		portNumber = atoi(argv[1]);
	}
	if ( (serverSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
		perror("socket error ");
		exit(1);	
	}

	if ( bindRecursive(serverSocket, portNumber, 0) == -1 ){
		printf("failed to bind for %d times, shut down\n", BINDING_TRY_LIMIT);
		exit(1);
	} 
	
	if ( listen(serverSocket, 0) == -1 ) {
		perror("listen error ");
		exit(1);
	}
	printf("waiting for connection...\n");
	clientSocket = accept(serverSocket, (struct sockaddr *) &clientaddr, &clientLen);
	if (clientSocket == -1) {
		perror("accept error ");
		exit(1);
	}
	while(1){
		memset(buf, '0', 255);
		count = recv(clientSocket, buf, 255, 0);
		if (count == -1) {
			perror("can't recieve ");
			close(clientSocket);
			break;
		} 
		buf[count] = '\0';
		if( strncmp(buf, identifyQuestion, strlen(identifyQuestion) ) == 0) {
			printf("client is connected\n");
			send(clientSocket, identifyAnswer, strlen(identifyAnswer), 0);	
		} else if ( buf[0] == 'C' ) {
			char *integer = buf + 1;
			windowSize = atoi(integer);
		} else if ( strncmp(buf, quitStr, strlen(quitStr)) == 0 ) {
			printf("Connection terminated\n");
			break;
		} else if ( buf[0] == 'R' ) {
			
			char fileDic[30];
			char *pt = fileDic;

			strcpy(pt, "data/");
			pt += strlen(pt);

			strcpy(pt, fileName[buf[1] - '1']);
			fl = fopen(fileDic, "r");

			printf("file name is : %s\n", fileDic);
			sendFile(clientSocket, fl, windowSize);		
		} 
	}
	close(clientSocket);
	close(serverSocket);

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
