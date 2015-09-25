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

#define MAKE_RESULT 1

void handler();
void timeoutHandler();
timer_t set_timer(long long);

int clientSocket;
int acknum = 0;
int timernum = 0;

int maxdiff;

timer_t t_id[1000];

int connectRecursive(char* hostName, int port, int numberofTry){
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
	
	clientSocket = socket(AF_INET, SOCK_STREAM, 0);
	state = connect(clientSocket, (struct sockaddr *)&clientaddr, socketLen);
	if (state == 0) {
		send(clientSocket,identifyQuestion, strlen(identifyQuestion), 0);
		alarm(3);
		int count; 
		printf("\twaiting for server's response...\n");
		if( (count = recv(clientSocket, buf, 255, 0)) <= 0 ){
			if (errno == EINTR) {
				printf("\tthis is not my server.\n");
				close(clientSocket);
				if (numberofTry < CONNECT_TRY_LIMIT) state = connectRecursive(hostName, port+1, numberofTry+1);
			}
		}
		alarm(0);
		if ( strncmp(buf, identifyAnswer, strlen(identifyAnswer)) == 0 ) {
			printf("Connection established\n");
			printf("\n");
		}
			
		
	} else if (state == -1) {
		perror("connect error ");
		if (numberofTry < CONNECT_TRY_LIMIT) state = connectRecursive(hostName, port+1, numberofTry+1);
	}
	return state;
}
int fullrecv(char * buffer, int size){
	int count = recv(clientSocket, buffer, size, 0);
	
	if (count <= 0) return -1;
	else if ( count == size ) return count;
	if (strncmp(buffer, transferFinished, strlen(transferFinished)) == 0) {
		memset(buffer, 0, size);	
		return 0;
	}
	char * next = buffer + count;
	int res = fullrecv(next, size - count);
	if (res == -1) return -1;
	else return res + count;
}

void receivingFile(FILE *file, int delay, int windowSize, int filenumber){
	char buffer[BUFFER_SIZE];
	struct timeval startTime, endTime;
	int kbytes = 0;
	int lastbytes = 0;


	struct sigaction sigact;

	sigemptyset(&sigact.sa_mask);
	sigaddset(&sigact.sa_mask, SIGALRM);
	sigact.sa_handler = &handler;
	sigact.sa_flags = SA_RESTART;
	sigaction(SIGALRM, &sigact, NULL);

	// for clear socket buffer and start timer!
	while(1){
		int count = recv(clientSocket, buffer, BUFFER_SIZE, 0);
		if ( count == BUFFER_SIZE ) {
			gettimeofday(&startTime, '\0');
			kbytes ++;
			fwrite(buffer, sizeof(char), count, file);
			t_id[timernum] = set_timer(delay);
			timernum++;
			break;
		}
	}

	while (1) {
		memset(buffer, 0, BUFFER_SIZE);

		int res = fullrecv(buffer, BUFFER_SIZE);
		if (res == -1) continue;

		fwrite(buffer, sizeof(char), abs(res), file);

		t_id[timernum] = set_timer(delay);
		if (timernum < 999) timernum++;
		else timernum = 0;
		//printf("timer running... timernum is : %d\n", timernum);

		if (res == BUFFER_SIZE) {
			kbytes ++;
			if (kbytes % 1000 == 0) printf("%d MB transmitted\n", kbytes / 1000);
		} else {
			lastbytes += res;
			usleep(delay * 1000);
			gettimeofday(&endTime, NULL);
			break;
		}
	}

	fclose(file);
	double elapsedTime = (double) (endTime.tv_sec - startTime.tv_sec) + (double)(endTime.tv_usec - startTime.tv_usec) / 1000000;
	double throughput = (double)(kbytes*1000+lastbytes)*8.0 / elapsedTime;
	printf("File transfer finished\n");
	printf("\n");
	printf("\tTransfered bits : %.3f KByte\n", (double)(kbytes*1000+lastbytes)/1000.0);
	printf("\tElapsed time : %f seconds\n", elapsedTime);
	printf("\tThroughput : %.3f mbps\n", throughput/1000000.0);	
	printf("\n");
	
#if MAKE_RESULT
	FILE *resfile;
	char res[100];
	sprintf(res, "%d\t%d\t%d\t%.3f\t%.3f\n", filenumber, windowSize, delay, elapsedTime, throughput/1000000.0);
	resfile = fopen("result.txt", "a");
	fputs(res, resfile);
	fclose(resfile);
#endif

}
int main (int argc, char **argv) {
   
	int portNumber, i;
	char *windowSize, *delay;
	char *hostName;
	char bufIn[16];
	FILE *fp;
	


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
			if( connectRecursive(hostName, portNumber, 0) == -1 ) {
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
					fp = fopen(fileDic, "wb");

					receivingFile(fp, atoi(delay), atoi(windowSize), bufIn[i] - '0');
					break;
				}
			}
			if (sending != 2) printf("n can be only 1, 2 or 3\n"); 
			
		} else if (bufIn[0] == 'F') {
			send(clientSocket, quitStr, strlen(quitStr), 0);
			printf("Connection terminated\n");
			break;
		} else {
			printf("\tundefined input : %s\n", bufIn);
			printf("\tList of inputs\n");
			printf("\tC : Connect to the server\n");
			printf("\tR n : Request to server to transmit file number n(n = 1, 2, 3)\n");
			printf("\tF: Finish the connection to the server\n");
			printf("\n");
		}
			
	}	
	close(clientSocket);
	return 0;
}

/*
 * handler()
 * Invoked by a timer.
 * Send ACK to the server
 */
void handler() {
	int count = send(clientSocket, ACK, strlen(ACK), 0);
	timer_delete(t_id[acknum]);
	if (acknum < 999) acknum ++;
	else acknum = 0;	
//	printf("ACK sent, acknum is : %d\n", acknum);
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
	long n_sec = (time % 1000) * 1000 * 1000;
    time_spec.it_value.tv_sec = sec;
    time_spec.it_value.tv_nsec = n_sec;

    timer_t t_id;
    if (timer_create(CLOCK_MONOTONIC, NULL, &t_id))
        perror("timer_create");
    if (timer_settime(t_id, 0, &time_spec, NULL))
        perror("timer_settime");

    return t_id;
}
