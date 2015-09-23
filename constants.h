
#define BUFFER_SIZE 		1000    // same as packet size
#define PORT_CHANGE_LIMIT	10
#define CONNECT_TRY_LIMIT 	PORT_CHANGE_LIMIT
#define BINDING_TRY_LIMIT       PORT_CHANGE_LIMIT


const char identifyQuestion[19] = 	"Are you my server?";
const char identifyAnswer[14] = 	"Yes my friend";
const char quitStr[5] =			"quit";
const char file1[20] = 			"TransferMe10.mp4";
const char file2[20] = 			"TransferMe10.mp4";
const char file3[20] = 			"TransferMe10.mp4";
const char fileName[3][20] = {
	"TransferMe10.mp4",
	"TransferMe20.mp4",
	"TransferMe30.mp4"
};
