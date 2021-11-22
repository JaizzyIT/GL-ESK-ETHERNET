#include "main.h"
#include "lwip.h"
#include "sockets.h"
#include "cmsis_os.h"
#include <string.h>

#define PORTNUM 5678UL

#if (USE_UDP_SERVER_PRINTF == 1)
#include <stdio.h>
#define UDP_SERVER_PRINTF(...) do { printf("[udp_server.c: %s: %d]: ",__func__, __LINE__);printf(__VA_ARGS__); } while (0)
#else
#define UDP_SERVER_PRINTF(...)
#endif

static struct sockaddr_in serv_addr, client_addr;
static int socket_fd;
static uint16_t nport;

enum{
	ON,
	OFF,
	TOGGLE
}ledCommand;

int parseString (const char *string, int len);

static int udpServerInit(void)
{
	socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (socket_fd == -1) {
		UDP_SERVER_PRINTF("socket() error\n");
		return -1;
	}

	nport = PORTNUM;
	nport = htons((uint16_t)nport);

	bzero(&serv_addr, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = nport;

	if(bind(socket_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))==-1) {
		UDP_SERVER_PRINTF("bind() error\n");
		close(socket_fd);
		return -1;
	}

	UDP_SERVER_PRINTF("Server is ready\n");

	return 0;
}

void StartUdpServerTask(void const * argument)
{
	int nbytes;
	char buffer[80];
	int addr_len;

	osDelay(5000);// wait 5 sec to init lwip stack

	if(udpServerInit() < 0) {
		UDP_SERVER_PRINTF("udpSocketServerInit() error\n");
		return;
	}

	for(;;)
	{
		  bzero(&client_addr, sizeof(client_addr));
		  addr_len = sizeof(client_addr);

		 nbytes = recvfrom(socket_fd, buffer, sizeof(buffer), 0, (struct sockaddr *) &client_addr, (socklen_t*) &addr_len);

		 if (nbytes == -1){
				UDP_SERVER_PRINTF("udpReceive error\n");
				sendto(socket_fd, "Receive error, try again\n", strlen("Receive error, try again\n"), 0, (struct sockaddr *)&client_addr, (socklen_t)addr_len);
				continue;
		 }
		 else if (nbytes <= 1){
			 	UDP_SERVER_PRINTF("udpReceive void message error\n");
				sendto(socket_fd, "udpReceive void message error\n", strlen("udpReceive void message error\n"), 0, (struct sockaddr *)&client_addr, (socklen_t)addr_len);
			 	continue;
		 }
		 else if (nbytes == sizeof(buffer)){
			 	UDP_SERVER_PRINTF("udpReceive buffer oversize error\n");
				sendto(socket_fd, "Buffer oversize error, try again\n", strlen("Buffer oversize error, try again\n"), 0, (struct sockaddr *)&client_addr, (socklen_t)addr_len);
			 	continue;
		 }
		 else{
				UDP_SERVER_PRINTF(buffer);
	//			sendto(socket_fd, buffer, nbytes, 0, (struct sockaddr *)&client_addr, (socklen_t)addr_len);
			 if (parseString(buffer, nbytes) < 0) {
				UDP_SERVER_PRINTF("Command error. Correct format: led <number>(0-4) <on/off/toggle>\n");
				sendto(socket_fd, "Command error. Correct format: led <number>(0-4) <on/off/toggle>\n", strlen("Command error. Correct format: led <number>(0-4) <on/off/toggle>\n"), 0, (struct sockaddr *)&client_addr, (socklen_t)addr_len);
				continue;
			}
			 else {
				UDP_SERVER_PRINTF("Command accepted\n");
				sendto(socket_fd, "Command accepted\n", strlen("Command accepted\n"), 0, (struct sockaddr *)&client_addr, (socklen_t)addr_len);
			 }
		 }
	}
}

int parseString (const char *string, int len){
	int scanStatus;
	uint8_t command;
	int ledNum = 0;
	char ledStr[3] = {0}, CommandStr[7] = {0};
	Led_TypeDef led[4] = {LED3, LED4, LED5, LED6};

	scanStatus = sscanf(string, "%s %u %s", ledStr, &ledNum, CommandStr);
	if (scanStatus != 3 || (strcmp(ledStr, "led") != 0)){
		return -1;
	}
	else if (ledNum < 0 || ledNum > 3){
		return -2;
	}

	if (strcmp(CommandStr, "on") == 0) command = ON;
	else if(strcmp(CommandStr, "off") == 0) command = OFF;
	else if(strcmp(CommandStr, "toggle") == 0) command = TOGGLE;
	else return -3;

	switch(command){
	case ON:
		BSP_LED_On(led[ledNum]);
		break;
	case OFF:
		BSP_LED_Off(led[ledNum]);
		break;
	case TOGGLE:
		BSP_LED_Toggle(led[ledNum]);
		break;
	default:
		break;
	}
	return 0;
}
