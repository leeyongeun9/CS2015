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

void handler();
void timeoutHandler();
timer_t set_timer(long long);

int clientSocket;
ackPacket *ack;

int connectRecursive(int socket, char* hostName, int port, int numberofTry){
	struct sockaddr_in clientaddr;
	int socketLen;
	int state;
	char buf[255];

	struct sigaction sigact;

	sigact.sa_handler = &timeoutHandler;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = 0;
	sigact.sa_flags |= SA_INTERRUPT;

	sigaction(SIGALRM, &sigact, NULL);

	clientaddr.sin_family = AF_INET;
	clientaddr.sin_addr.s_addr = inet_addr(hostName);
	clientaddr.sin_port = htons(port);

	socketLen = sizeof(clientaddr);
	printf("\n");	
	printf("------port number : %d------\n", port);
        printf("\ttrying to connect...\n");
	
	state = connect(socket, (struct sockaddr *)&clientaddr, socketLen);
	if (state == 0) {
		send(socket,identifyQuestion, strlen(identifyQuestion), 0);
		alarm(3);
		int count; 
		printf("\twaiting for server's response...\n");
		if( (count = recv(socket, buf, 255, 0)) <= 0 ){
			if (errno == EINTR) {
				printf("\tthis is not my server.\n");
				close(socket);
				if (numberofTry < CONNECT_TRY_LIMIT) state = connectRecursive(socket, hostName, port+1, numberofTry+1);
			}
		}
		alarm(0);
		if ( strncmp(buf, identifyAnswer, strlen(identifyAnswer)) == 0 ) {
			printf("Connection established\n");
			printf("\n");
		}
			
		
	} else if (state == -1) {
		perror("connect error ");
		if (numberofTry < CONNECT_TRY_LIMIT) state = connectRecursive(socket, hostName, port+1, numberofTry+1);
	}
	return state;
}
void receivingFile(int socket, FILE *file, int delay){
	myPacketBuffer *buf;
	char buffer[BUFFER_SIZE];
	clock_t startTime, endTime;
	int rn=0;
	int isStarted = 0;
	int kbits = 0;
	int lastbits = 0;
	
	buf = malloc(sizeof(myPacketBuffer));
	ack = malloc(sizeof(ackPacket));

	ack->length = 9;
	strncpy(ack->msg, ACK, strlen(ACK));
	printf("ack->msg is %s\n", ack->msg);
	printf("myPackeyBuffer size is : %d\n", sizeof(myPacketBuffer));
	struct sigaction sigact;

	sigemptyset(&sigact.sa_mask);
	sigaddset(&sigact.sa_mask, SIGALRM);
	sigact.sa_handler = &handler;
	//sigact.sa_flags = 0;
 	//sigact.sa_flags |= SA_INTERRUPT;
	sigaction(SIGALRM, &sigact, NULL);
	while (1) {
	//	printf("ready to recieve\n");
		int count = recv(socket, buf, sizeof(myPacketBuffer), MSG_WAITALL);
		printf("buf->rn : %d, buf->length : %d, count is : %d\n", buf->sn, buf->length, count);
	//	printf("count is : %d\n", count);
		if (isStarted ==0){
			startTime = clock();
			isStarted = 1;
		}

		if (buf->sn == rn && buf->length + HEADER_SIZE == count) {
			if (strncmp(buf->pack, transferFinished, strlen(transferFinished)) == 0 ) {
				usleep(delay * 1000);
				endTime = clock();
				break;		
			}
			if (buf->length == BUFFER_SIZE) kbits ++;
			else lastbits += buf->length;
				
			fputs(buf->pack, file); 
			if (kbits % 8000 == 0) printf("%dMB transfered\n", kbits / 8000);
			rn++;
			set_timer(delay);
			
		}
		
//		printf("first is : %c\n", buffer[0]);
	/*
		if (isStarted==0){
			startTime = clock();
			isStarted = 1;
		}

		if (count == -1) {
			if (errno == EINTR) {
				continue;
			}
		} 
	*/
		/*if (errno == EINTR) {
			char *pt = buffer[count];
			int last = recv(socket, pt, BUFFER_SIZE-count, 0);
			printf ("count is : %d, last is : %d\n", count, last);
			count = count + last;
		}*/
/*
		if (count < BUFFER_SIZE) {
			printf("count is : %d\n", count);
			printf("errno is : %d\n", errno);
			buffer[count] = '\0';
			if (strncmp(buffer, transferFinished, strlen(transferFinished)) == 0) {
				usleep(delay * 1000);
				endTime = clock();
				break;		
			}
			lastbits += count;
		} else kbits ++;
*/
	//	if (kbits % 8000 == 0) printf("%dMB transfered\n", kbits / 8000);

	//	fputs(buffer, file);
	//	set_timer(delay);
//		printf("set timer\n");
//		printf("i : %d\n", i);
	}
	float elapsedTime = (float) (endTime - startTime) / CLOCKS_PER_SEC;
	float throughput = (float)(kbits*1000+lastbits) / elapsedTime;
	printf("\tFile translate finished\n");
	printf("\tTransfered bits : %.3f kbit\n", (float)(kbits*1000+lastbits)/1000);
	printf("\tElapsed time : %.3f\n", elapsedTime);
	printf("\tThroughput : %f\n", throughput);	

}
int main (int argc, char **argv) {
   
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

	while(1){
		printf("Enter your command : ");
		memset(bufIn, '0', 16);
		fgets(bufIn, 16, stdin);
		bufIn[strlen(bufIn)-1] = '\0';
		if (bufIn[0] == 'C'){
			if( connectRecursive(clientSocket, hostName, portNumber, 0) == -1 ) {
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
					char message[10];
					message[0] = 'R';
					message[1] = bufIn[i];
					message[2] = '\0';
					sending = send(clientSocket, message, strlen(message), 0);

					char fileDic[30];
					char *pt = fileDic;

					strcpy(pt, "data/");
					pt += strlen(pt);

					strcpy(pt, fileName[bufIn[i] - '1']);
					fp = fopen(fileDic, "w");

					receivingFile(clientSocket, fp, atoi(delay));
					break;
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
	// Set Handler for timers

	// Timer example

	// TODO: Create a socket for a client
	//      connect to a server
	//      set ACK delay
	//      set server window size
	//      specify a file to receive
	//      finish the connection

	// TODO: Receive a packet from server
	//      set timer for ACK delay
	//      send ACK packet back to server (usisng handler())
	//      You may use "ack_packet"

	// TODO: Close the socket
	close(clientSocket);
	return 0;
}

/*
 * handler()
 * Invoked by a timer.
 * Send ACK to the server
 */
void handler() {
	printf("ack sending...\n");
	ack->rn = ack->rn + 1;
	int count = send(clientSocket, ack, sizeof(ackPacket), 0);
	printf("send ack, count : %d, ack rn : %d, ack msg : %s\n", count, ack->rn, ack->msg);
}
	
void timeoutHandler(){
	printf("timeout\n");
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
