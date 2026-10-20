#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include "data.h"
#include "log.h"
#include "server.h"

// #define IP_APU "10.59.152.52"
// #define IP_FPU "10.59.152.51"
// #define IP_DB "10.59.152.52"
#define IP_APU "127.0.0.1"
#define IP_FPU "127.0.0.1"
#define IP_DB "127.0.0.1"
#define IP_COUNTER "10.59.152.222"
#define PORT_COUNTER 8082

#define PORT_APU 8080
#define PORT_FPU 9002
#define PORT_DB 8081
#define PORT_SERVER 8087
#define BUF_SIZE 256

typedef enum
{
    RC_OK = 0,
    RC_SOCK = -1,
    RC_IP = -2,
    RC_CONNECT = -3
} exit_code;

int main(void)
{
    int apu_sock = -1, fpu_sock = -1, db_sock = -1;
    int counter_server_sock = -1, counter_client_sock = -1;
    exit_code rc = RC_OK;

    apu_sock = connect_to_server(IP_APU, PORT_APU);
    fpu_sock = connect_to_server(IP_FPU, PORT_FPU);
    db_sock = connect_to_server(IP_DB, PORT_DB);

    if (apu_sock < 0 || fpu_sock < 0)
    {
        print_err_all_connect();
        rc = RC_CONNECT;
    }
    else
    {
        counter_server_sock = create_server(PORT_SERVER);
        if (counter_server_sock < 0)
            rc = RC_SOCK;
        else
        {
            printf("[INFO] Сервер управления запущен на порту %d\n", PORT_SERVER);
            
            fd_set readfds;
            struct timeval timeout;
            char input[BUF_SIZE];
            int running = 1;
            
            while (running)
            {
                FD_ZERO(&readfds);
                FD_SET(counter_server_sock, &readfds);
                FD_SET(STDIN_FILENO, &readfds);
                
                timeout.tv_sec = 1;
                timeout.tv_usec = 0;
                
                int max_fd = (counter_server_sock > STDIN_FILENO) ? counter_server_sock : STDIN_FILENO;
                int activity = select(max_fd + 1, &readfds, NULL, NULL, &timeout);
                
                if (activity < 0)
                    break;
                
                if (FD_ISSET(counter_server_sock, &readfds))
                {
                    counter_client_sock = accept(counter_server_sock, NULL, NULL);
                    if (counter_client_sock >= 0)
                    {
                        handle_counter_client(counter_client_sock, apu_sock, fpu_sock, db_sock);
                        counter_client_sock = -1;
                    }
                }

                if (FD_ISSET(STDIN_FILENO, &readfds))
                {
                    printf("Введите команду (или 'exit' для выхода): ");
                    if (fgets(input, sizeof(input), stdin) != NULL)
                    {
                        input[strcspn(input, "\n")] = '\0';
                        
                        if (strcmp(input, "exit") == 0)
                            running = 0;
                        else if (strlen(input) > 0)
                        {
                            command_t cmd = parse_cmd(input);
                            if (cmd.args_count > 0)
                            {
                                perform_cmd(&cmd, apu_sock, fpu_sock, db_sock);
                                free_command(&cmd);
                            }
                            else
                            {
                                int counter_sock = connect_to_server(IP_COUNTER, PORT_COUNTER);
                                if (counter_sock >= 0)
                                {
                                    send_line(counter_sock, input);
                                    char response[BUF_SIZE] = {0};
                                    int received = 0;
                                    char *ptr = response;
                                    
                                    while (received < BUF_SIZE - 1)
                                    {
                                        int bytes = recv(counter_sock, ptr, 1, 0);
                                        if (bytes <= 0) 
                                            break;
                                        if (*ptr == '\n' || *ptr == '\0')
                                        {
                                            *ptr = '\0';
                                            break;
                                        }
                                        ptr++;
                                        received++;
                                    }
                                    printf("[COUNTER] Ответ: %s\n", response);
                                    close(counter_sock);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    if (apu_sock >= 0) 
        close(apu_sock);
    if (fpu_sock >= 0) 
        close(fpu_sock);
    if (db_sock >= 0) 
        close(db_sock);
    if (counter_client_sock >= 0) 
        close(counter_client_sock);
    if (counter_server_sock >= 0) 
        close(counter_server_sock);
    
    return rc;
}