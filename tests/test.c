#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define APU_PORT 8080
#define FPU_PORT 9002
#define DB_PORT 8081

void* apu_server(void* arg) 
{
    int server = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {AF_INET, htons(APU_PORT), INADDR_ANY};
    
    bind(server, (struct sockaddr*)&addr, sizeof(addr));
    listen(server, 5);
    printf("APU сервер запущен на порту %d\n", APU_PORT);
    
    int client = accept(server, NULL, NULL);
    printf("APU: клиент подключен\n");
    
    while(1) {
        char buf[256] = {0};
        recv(client, buf, 255, 0);
        printf("APU получил: %s\n", buf);
        send(client, "OK\n", 3, 0);
    }
    
    return NULL;
}

void* fpu_server(void* arg) 
{
    int server = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {AF_INET, htons(FPU_PORT), INADDR_ANY};
    
    bind(server, (struct sockaddr*)&addr, sizeof(addr));
    listen(server, 5);
    printf("FPU сервер запущен на порту %d\n", FPU_PORT);
    
    int client = accept(server, NULL, NULL);
    printf("FPU: клиент подключен\n");
    
    while(1) {
        char buf[256] = {0};
        recv(client, buf, 255, 0);
        printf("FPU получил: %s\n", buf);
        send(client, "3.14\n", 5, 0);
    }
    
    return NULL;
}

void* db_server(void* arg) 
{
    int server = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {AF_INET, htons(DB_PORT), INADDR_ANY};
    
    bind(server, (struct sockaddr*)&addr, sizeof(addr));
    listen(server, 5);
    printf("DB сервер запущен на порту %d\n", DB_PORT);
    
    int client = accept(server, NULL, NULL);
    printf("DB: клиент подключен\n");
    
    while(1) {
        char buf[256] = {0};
        recv(client, buf, 255, 0);
        printf("DB получил: %s\n", buf);
        send(client, "VALUE\n", 6, 0);
    }
    
    return NULL;
}

int main() 
{
    pthread_t t1, t2, t3;
    
    pthread_create(&t1, NULL, apu_server, NULL);
    pthread_create(&t2, NULL, fpu_server, NULL);
    pthread_create(&t3, NULL, db_server, NULL);
    
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);
    
    return 0;
}