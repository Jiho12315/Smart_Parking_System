
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cjson/cJSON.h>
#include <wiringPi.h>
#include <pthread.h>



#define SERVER_IP "192.168.199.29"
#define PORT 8080
#define BUFFER_SIZE 1024

int segments[] = {0,1,2,3,4,5,6,}; // a, b, c, d, e, f, g (dp는 사용 안함)
int digits[] = {10,8,9};            // 디스플레이 공통 핀 (1층, 2층, 총합)
int parking[8]={0,0,0,0,0,0,0,0};


// 숫자에 따른 세그먼트 패턴 (a-g)
int numToSegments[10][7] = {                                                                                                                                                                                                                                                                          
    {1, 1, 1, 1, 1, 1, 0}, // 0
    {0, 1, 1, 0, 0, 0, 0}, // 1
    {1, 1, 0, 1, 1, 0, 1}, // 2
    {1, 1, 1, 1, 0, 0, 1}, // 3
    {0, 1, 1, 0, 0, 1, 1}, // 4
    {1, 0, 1, 1, 0, 1, 1}, // 5
    {1, 0, 1, 1, 1, 1, 1}, // 6
    {1, 1, 1, 0, 0, 0, 0}, // 7
    {1, 1, 1, 1, 1, 1, 1}, // 8
    {1, 1, 1, 1, 0, 1, 1}  // 9
};

// 초기화 함수
void setup() {
    wiringPiSetup();
    for (int i = 0; i < 7; i++) {
        pinMode(segments[i], OUTPUT);
        digitalWrite(segments[i], HIGH);
    }
    for (int i = 0; i < 3; i++) {
        pinMode(digits[i], OUTPUT);
        digitalWrite(digits[i], LOW); // 공통 핀 비활성화
    }
}

// 특정 디스플레이에 숫자 출력
void displayNumber(int digit, int number) {
    // 모든 디스플레이 비활성화
    for (int i = 0; i < 3; i++) {
        digitalWrite(digits[i], HIGH);
    }
    digitalWrite(7, HIGH);
    digitalWrite(8, HIGH);
    digitalWrite(9, HIGH);
    // 선택된 디스플레이 활성화
    digitalWrite(digits[digit], LOW);

    // 숫자에 따른 세그먼트 출력
    for (int i = 0; i < 7; i++) {
        digitalWrite(segments[i], numToSegments[number][i]);
    }

    // 짧은 시간 동안 표시 (멀티플렉싱)
    usleep(7500); // 5ms
}

// 남은 자리 계산
void calculateRemaining(int *arr, int size, int *firstFloor, int *secondFloor, int *total) {
    *firstFloor = 0;
    *secondFloor = 0;
    for (int i = 0; i < size; i++) {
        if (i < 2 && arr[i] == 0) {
            (*firstFloor)++;
        } else if (i >= 2 && arr[i] == 0) {
            (*secondFloor)++;
        }
    }
    *total = *firstFloor + *secondFloor;
}

// 메인 루프
void *loop() {
    int firstFloor, secondFloor, total;

    while (1) {
        calculateRemaining(parking, 4, &firstFloor, &secondFloor, &total);
	

        // 1층 남은 자리
        displayNumber(0, firstFloor);
        // 2층 남은 자리
	displayNumber(1, secondFloor);
        // 총 남은 자리
        displayNumber(2, total);

        
    }
}


int main() {
    int sock;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    wiringPiSetup();	
    setup();
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
    cJSON_AddNumberToObject(classification_json, "class",2);
    char *json_socket = cJSON_Print(classification_json);
    
    send(sock,json_socket, strlen(json_socket),0);
    
    int bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
		if (bytes_received > 0) {
			buffer[bytes_received] = '\0';
			printf("Server response: %s\n", buffer);
		}
	// 쓰레드 생성
	
	
	while(1){
		
		int bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
		
		if (bytes_received > 0) {
			buffer[bytes_received] = '\0';
			//printf("Server response: %s\n", buffer);
		}
		
		cJSON *json = cJSON_Parse(buffer);
		if(json == NULL) 
		{
			printf("Error parsing JSON\n");
			continue;
		}
		
		 cJSON *park_state = cJSON_GetObjectItem(json, "index");
				
		for (int i = 0; i < 8; i++) {
			cJSON *item = cJSON_GetArrayItem(park_state, i);
			if (cJSON_IsNumber(item)) {
				parking[i] = item->valueint;
			} else {
				printf("Error: index[%d] is not a number\n", i);
			}
		}
		
		for (int i=0;i<4;i++)
		{
		    printf("%d ",parking[i]);
		}
		printf("\n");
		fflush(stdout);
		
		pthread_t thread_id;
		pthread_create(&thread_id,NULL,loop,NULL);
		// 메모리 해제
		cJSON_Delete(json);
	}
    close(sock);

    return 0;
}


