#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include <cjson/cJSON.h>
#include <wiringPi.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int parking[8] = {0,0,0,0,
                  0,0,0,0};

typedef struct parking_time{
    int index;
    time_t start;
}p_time;



pthread_mutex_t lock;
pthread_mutex_t lock2;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int task_completed =0;
/*
int detection()
{
    int start_time, end_time;
    float distance;
    pinMode(TRIG,OUTPUT);
    pinMode(ECHO,INPUT);
    digitalWrite(TRIG,LOW);
    delay(500);
    digitalWrite(TRIG,HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG,LOW);
    while (digitalRead(ECHO) == 0);
    start_time = micros();
    while(digitalRead(ECHO) == 1);
    end_time = micros();
    distance = (end_time - start_time) / 29. / 2.;
    //printf("distance : %.2f cm -> ",distance);
    if(distance < 10.0f) return 0; //주차된 상태
    else return 1; //빈 상태
}
*/
void edit_parking_lot(int index, int state)
{
    pthread_mutex_lock(&lock);
    parking[index] = state;
   
    printf("\n present parking state\n");
    fflush(stdout);
    for (int i = 0;i<8;i++)
    {
        printf("%d ",parking[i]);
    }
    printf("\n\n");
    fflush(stdout);
   
    pthread_mutex_unlock(&lock);
   
   
}
       
void *handle_client(void *client_socket) {
    printf("client Thread start....\n\n");
    fflush(stdout);
   
    int sock = *(int *)client_socket;
    free(client_socket);  // 동적으로 할당된 소켓 메모리 해제
    char buffer[BUFFER_SIZE];

    while (1) {
        // 클라이언트로부터 데이터 수신
        int bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) {
            printf("Client disconnected\n");
            break;
        }

        buffer[bytes_received] = '\0';
        printf("Received from client: %s\n", buffer);

        // JSON 파싱
        cJSON *json = cJSON_Parse(buffer);
        if (json == NULL) {
            printf("Error parsing JSON\n");
            continue;
        }
        int index;
        int state;
       
        // JSON 데이터 처리
        cJSON *message = cJSON_GetObjectItem(json, "message");
        if (cJSON_IsString(message)) {
            printf("Message: %s\n", message->valuestring);
        }
       
        cJSON *jindex = cJSON_GetObjectItem(json,"index");
        index = jindex->valueint;
       
        cJSON *jstate = cJSON_GetObjectItem(json,"state");
        state = jstate->valueint;
       
        edit_parking_lot(index,state);

        // JSON 응답 생성
        cJSON *response = cJSON_CreateObject();
        cJSON_AddStringToObject(response, "status", "OK");
        char *response_string = cJSON_Print(response);

        // 클라이언트로 응답 전송
        send(sock, response_string, strlen(response_string), 0);

        // 메모리 해제
        cJSON_Delete(json);
        cJSON_Delete(response);
        free(response_string);
    }

    close(sock);
    return NULL;
}

void *handle_display(void *client_socket) {
    printf("display Thread start....\n\n");
    fflush(stdout);
   
    int sock = *(int *)client_socket;
    free(client_socket);  // 동적으로 할당된 소켓 메모리 해제
    char buffer[BUFFER_SIZE];

    while (1) {
       
        cJSON  *json = cJSON_CreateObject();
        cJSON *park_state = cJSON_CreateArray();
       
        pthread_mutex_lock(&lock);
        for(int i = 0;i<8;i++)
        {
            cJSON_AddItemToArray(park_state, cJSON_CreateNumber(parking[i]));
        }
        pthread_mutex_unlock(&lock);
       
        cJSON_AddItemToObject(json,"index",park_state);
        char *json_socket = cJSON_Print(json);
       
        send(sock,json_socket,strlen(json_socket),0);
       
        sleep(2);
    }

    close(sock);
    return NULL;
}

p_time timedata[8];

void *handle_calc(void *client_socket) {
    printf("calc Thread start....\n\n");
    fflush(stdout);
   
    int sock = *(int *)client_socket;
    free(client_socket);  // 동적으로 할당된 소켓 메모리 해제
    char buffer[BUFFER_SIZE];
    while(1) {
        // 클라이언트로부터 데이터 수신
        int bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) {
            printf("Client disconnected\n");
            break;
        }
       
        time_t cur_time = time(NULL);
        buffer[bytes_received] = '\0';
        printf("Received from client: %s\n", buffer);

        // JSON 파싱
        cJSON *json = cJSON_Parse(buffer);
        if (json == NULL) {
            printf("Error parsing JSON\n");
            continue;
        }
        int price = -1;
        int number;
        int time_index = -1;
        cJSON *jnumber = cJSON_GetObjectItem(json,"CarNumber");
        number = jnumber->valueint;
       
        //번호가 주차장에 있는지 확인
        for (int i=0;i<8;i++)
        {
            if(timedata[i].index == number)
            {
                time_index = i;
                printf("number %d is ready\n");
                fflush(stdout);
            }

        }
        time_t second;
        //만약 주차장에 번호가 없으면 들어오는 차
        if(time_index == -1)
        {
            for (int i=0;i<8;i++)
            {
                if (timedata[i].start == -1)
                {
                    timedata[i].index = number;
                    timedata[i].start = cur_time;
                    break;
                }
            }
        }
        else //만약 번호가 있다면 나가는 차
        {
            second = cur_time - timedata[time_index].start;
            price = cur_time - timedata[time_index].start;
            timedata[time_index].index = -1;
            timedata[time_index].start = -1;
            printf("price : %d\n\n",price);
            fflush(stdout);
        }
       
        for (int i = 0; i<8;i++)
        {
            if (timedata[i].index == -1) {continue;}
            printf("CarNumber : %d\n",timedata[i].index);
            printf("starttime : %d\n\n",timedata[i].start);
            fflush(stdout);
        }

        // JSON 응답 생성
        cJSON *response = cJSON_CreateObject();
        cJSON_AddNumberToObject(response,"price",price);
        char *response_string = cJSON_Print(response);
       
        // 클라이언트로 응답 전송
        send(sock, response_string, strlen(response_string), 0);

        pthread_mutex_lock(&lock2);
        task_completed=1;
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&lock2);
        // 메모리 해제
        cJSON_Delete(json);
        cJSON_Delete(response);
        free(response_string);
    }
    close(sock);
    return NULL;
}
void *handle_barrier(void *client_socket) {
    printf("barrier Thread start....\n\n");
    fflush(stdout);
   
    int sock = *(int *)client_socket;
    free(client_socket);  // 동적으로 할당된 소켓 메모리 해제
    char buffer[BUFFER_SIZE];
   
        while (1) {
        // 작업 완료 후 수행할 작업
        pthread_mutex_lock(&lock2);
        while( task_completed ==0)
        {
            pthread_cond_wait(&cond,&lock2);
        }
       
        printf("Barrier thread detected task completion by Class 3!\n");
        task_completed = 0;  // 상태 초기화
        pthread_mutex_unlock(&lock2);

        // 차단봉 작업 수행
        printf("Raising barrier...\n");
        cJSON *response = cJSON_CreateObject();
        cJSON_AddStringToObject(response, "status", "OK");
        char *response_string = cJSON_Print(response);

        // 클라이언트로 응답 전송
        send(sock, response_string, strlen(response_string), 0);
        printf("send ok\n");
        fflush(stdout);
        sleep(2);  // 예: 차단봉을 올리는 데 걸리는 시간
        printf("Barrier raised!\n");

}
}


void *handle_signal(void *client_socket) {
    printf("signal Thread start....\n\n");
    fflush(stdout);
   
    int sock = *(int *)client_socket;
    free(client_socket);  // 동적으로 할당된 소켓 메모리 해제
    char buffer[BUFFER_SIZE];
   
    while (1) {
    int take_picture =1;
    while (take_picture >= 1)
    {
        //초음파 거리가 일정거리 이하가 되면 break
        printf("input something : ");
        scanf("%d",&take_picture);
        delay(2000);
    }
   
    printf("close!!\n");
    fflush(stdout);
   
    cJSON *picture = cJSON_CreateObject();
    cJSON_AddStringToObject(picture,"state","OK");
    char *picture_string= cJSON_Print(picture);
   
    send(sock, picture_string, strlen(picture_string), 0);
       
    }

    close(sock);
    return NULL;
}







int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char ser_buffer[BUFFER_SIZE];

    pthread_mutex_init(&lock, NULL);
   
    for (int i = 0 ;i<8;i++)
    {
        timedata[i].index = -1;
        timedata[i].start = -1;
    }

    // 소켓 생성
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // 주소와 포트 바인딩
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // 연결 대기
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);

    while (1) {
        // 새 클라이언트 연결 수락
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (new_socket < 0) {
            perror("Accept failed");
            continue;
        }
        printf("New client connected\n");
   
        int bytes_received = recv(new_socket, ser_buffer, sizeof(ser_buffer) - 1, 0);
        if (bytes_received <= 0) {
            printf("Client disconnected\n");
            break;
        }

        ser_buffer[bytes_received] = '\0';
        printf("Received from client: %s\n", ser_buffer);

        // JSON 파싱
        cJSON *json = cJSON_Parse(ser_buffer);
        if (json == NULL) {
            printf("Error parsing JSON\n");
            continue;
        }
       
        int class;
        cJSON *jclass = cJSON_GetObjectItem(json,"class");
        class = jclass->valueint;
       
       
        cJSON *response = cJSON_CreateObject();
        cJSON_AddStringToObject(response, "status", "OK");
        char *response_string = cJSON_Print(response);

        // 클라이언트로 응답 전송
        send(new_socket, response_string, strlen(response_string), 0);
       
               
           
        // 클라이언트를 처리할 스레드 생성
        pthread_t thread_id;
        int *client_socket = malloc(sizeof(int));
        *client_socket = new_socket;
       
        printf("Class : %d \n\n",class);
       
        if (class == 1)
        {
            pthread_create(&thread_id, NULL, handle_client, client_socket);
        }
        else if (class == 2)
        {
            pthread_create(&thread_id, NULL, handle_display, client_socket);
        }
        else if (class ==3)
        {
            pthread_create(&thread_id, NULL, handle_calc, client_socket);
        }
        else if (class ==4)
        {
            pthread_create(&thread_id, NULL, handle_barrier, client_socket);
        }
        else if (class ==5)
        {
            pthread_create(&thread_id, NULL, handle_signal, client_socket);
        }

        // 메인 스레드와 독립적으로 실행되도록 스레드 분리
        pthread_detach(thread_id);
    }

    close(server_fd);
    pthread_mutex_destroy(&lock);
    return 0;
}

