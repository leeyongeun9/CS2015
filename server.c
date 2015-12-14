#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <signal.h>
#include "constants.h"

void handler();
timer_t set_timer(long long);

int minSN = 0, maxSN = -1;
FILE *fl;

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

void sendFile(int socket, int windowSize) {
	myPacketBuffer *buf;
	char *pt;
	ackPacket *ackbuf;

        struct sigaction sigact;

        sigemptyset(&sigact.sa_mask);
        sigaddset(&sigact.sa_mask, SIGALRM);
        sigact.sa_handler = &handler;
        sigaction(SIGALRM, &sigact, NULL);
	
	buf = malloc(sizeof(myPacketBuffer));
	ackbuf = malloc(sizeof(ackPacket));

	while(fgets(buf->pack, BUFFER_SIZE, fl) != '\0') {
		printf("ready...\n");
		if (minSN + windowSize -1 > maxSN) {
			maxSN++;
			buf->sn = maxSN;
			buf->length = sizeof(buf->pack);
			int count = send(socket, (void *)buf, sizeof(*buf), 0);
			printf("buf->sn : %d, buf->length : %d, count : %d\n", buf->sn, buf->length, count);
		} else {
			printf("1\n");
			set_timer(1000);
			int count = recv(socket, ackbuf, sizeof(ackPacket), MSG_WAITALL);
			if (count == -1) continue;
			printf("ack->rn : %d, ack->msg : %s, count : %d\n", ackbuf->rn, ackbuf->msg, count);
			if ( strncmp(ackbuf->msg, ACK, strlen(ACK)) == 0  && ackbuf->rn > minSN ) {
				alarm(0);
				printf("4\n");
				minSN = ackbuf->rn;
				printf("ok\n");
			}
		}
		memset(buf, '0', sizeof(*buf));
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
    printf("received data : %s\n", buf);
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
			
		} 
	}
	close(clientSocket);
	close(serverSocket);
	return 0;
}


/*
 * handler()
 * Invoked by a timer.
 * Send ACK to the server
 */
void handler() {
	printf("maxSN is : %d, minSN is : %d\n", maxSN, minSN);
	fl = fl - sizeof(myPacketBuffer) * (maxSN - minSN + 1);
	maxSN = minSN;
	printf("no problem\n");
	maxSN = minSN;
}



/*
 * set_timer()
 * set timer in msec
 */
timer_t set_timer(long long time) {
    struct itimerspec time_spec = {.it_interval = {.tv_sec=0,.tv_nsec=0},
                                .it_value = {.tv_sec=0,.tv_nsec=0}};

        int sec = time / 1000;
        long n_sec = (time % 1000) * 1000;
    time_spec.it_value.tv_sec = sec;
    time_spec.it_value.tv_nsec = n_sec;

    timer_t t_id;
    if (timer_create(CLOCK_MONOTONIC, NULL, &t_id))
        perror("timer_create");
    if (timer_settime(t_id, 0, &time_spec, NULL))
        perror("timer_settime");

    return t_id;
}
