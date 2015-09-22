#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <signal.h>

#define BUFFER_SIZE 1000    // same as packet size
#define CONNECT_TRY_LIMIT 10

void handler();
timer_t set_timer(long long);

int connectRecursive(int socket, char* hostName, int port, int numberofTry){
	struct sockaddr_in clientaddr;
	int socketLen;
	int state;

	clientaddr.sin_family = AF_INET;
	clientaddr.sin_addr.s_addr = inet_addr(hostName);
	clientaddr.sin_port = htons(port);

	socketLen = sizeof(clientaddr);
	
	printf("------port number : %d------\n", port);
        printf("trying to connect...\n");
	
	state = connect(socket, (struct sockaddr *)&clientaddr, socketLen);
	if (state == -1) {
		perror("connect error ");
		if (numberofTry < CONNECT_TRY_LIMIT) state = connectRecursive(socket, hostName, port+1, numberofTry+1);
	} else {
		printf("connecting success\n");
	}
	return state;
}
int main (int argc, char **argv) {
   
	int clientSocket, i;
	int portNumber, windowSize, ACKDelay;
	char *hostName;
	char bufIn[16];
	
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
	for (i = 3 ; i < 5 ; i = i + 2 ) {
		if (strncmp(argv[i], "-w", 2)) windowSize = atoi(argv[i+1]);
		if (strncmp(argv[i], "-d", 2)) ACKDelay = atoi(argv[i+1]);
	}
	clientSocket = socket(AF_INET, SOCK_STREAM, 0);

	while(1){
		fgets(bufIn, 16, stdin);
		bufIn[strlen(bufIn)-1] = '0';
		if (bufIn[0] == 'C'){
			if( connectRecursive(clientSocket, hostName, portNumber, 0) == -1 ) {
				printf("failed to connect for %d times, shut down\n", CONNECT_TRY_LIMIT);
				exit(1);
			} else {
				printf("Connection established\n");
			}
		} else if (bufIn[0] == 'R') {
		} else if (bufIn[0] == 'F') {
		} else {
			printf("\tundefined input : %s\n", bufIn);
			printf("\t C : Connect to the server\n");
			printf("\t R n : Request to server to transmit file number n\n");
			printf("\t F: Finish the connection to the server\n");
		}
			
	}	
	// Set Handler for timers
	struct sigaction sigact;
	sigemptyset(&sigact.sa_mask);
	sigaddset(&sigact.sa_mask, SIGALRM);
	sigact.sa_handler = &handler;
	sigaction(SIGALRM, &sigact, NULL);

	set_timer(10000);

	set_timer(1000);
	// Timer example


	while(1){}

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
	return 0;
}

/*
 * handler()
 * Invoked by a timer.
 * Send ACK to the server
 */
void handler() {
    printf("Hi\n");
	// TODO: Send an ACK packet
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
