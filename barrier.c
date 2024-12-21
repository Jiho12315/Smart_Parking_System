#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <wiringPi.h>
#include <wiringSerial.h>
#include <arpa/inet.h>
#include <cjson/cJSON.h>

#define BUFFER_SIZE 1024
#define PORT 8080
#define PWM 18 
#define SERVER_IP "192.168.199.29"


int main() //duty 30이면 1.5ms duty 50이면 2.5ms 1duty이면 4.5도
{

	wiringPiSetupGpio();
	pinMode(PWM,PWM_OUTPUT);
    
    pwmSetMode(PWM_MODE_MS);
    pwmSetRange(400);
    pwmSetClock(960);
	pwmWrite(PWM,30);
    int sock;
    int index=0;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];


    if (wiringPiSetupGpio() == -1) exit(1);
    // 소켓 생성
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        return 1;
    }

    // 서버 주소 설정
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;	
    server_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("Invalid address");
        close(sock);
        return 1;
    }

    // 서버에 연결
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sock);
        return 1;
    }

    printf("Connected to server\n");
    
    cJSON *classification_json = cJSON_CreateObject();
    cJSON_AddNumberToObject(classification_json, "class",4);
    char *json_socket = cJSON_Print(classification_json);
    
    send(sock,json_socket, strlen(json_socket),0);
    
    int bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
		if (bytes_received > 0) {
			buffer[bytes_received] = '\0';
			printf("Server response: %s\n", buffer);
		}
    
	 while (1) {
        // 서버로부터 데이터 수신
        int bytes_received = read(sock, buffer, sizeof(buffer) - 1);
	printf("receive complete\n");
	fflush(stdout);
        if (bytes_received <= 0) {
            printf("Server disconnected or error occurred\n");
            break;
        }
	
	//차단기 열었다가 닫힘
	pwmWrite(PWM,50);
	printf("차단기 열림");
	sleep(3);
	pwmWrite(PWM,30);
	printf("차단기 닫힘\n");	

	}
	return 0;
}	
 
