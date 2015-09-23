
#define BUFFER_SIZE 		1000    // same as packet size
#define HEADER_SIZE		8	
#define PORT_CHANGE_LIMIT	10
#define CONNECT_TRY_LIMIT 	PORT_CHANGE_LIMIT
#define BINDING_TRY_LIMIT       PORT_CHANGE_LIMIT


const char identifyQuestion[19] = 	"Are you my server?";
const char identifyAnswer[14] = 	"Yes my friend";

const char ACK[10] = 			"ACKACKACK";
const char transferFinished[10] = 	"finished";

const char quitStr[5] =			"quit";

const char fileName[3][20] = {
	"TransferMe10.mp4",
	"TransferMe20.mp4",
	"TransferMe30.mp4"
};

typedef struct myPacket {
	unsigned long sn;
	unsigned long length;
	char pack[BUFFER_SIZE];
} myPacketBuffer;

typedef struct ackp {
	unsigned long rn;
	unsigned long length;
	char msg[10];
}ackPacket;
