#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include <ctype.h>
#include "data.h"
#include "log.h"
#include "server.h"

#define BUF_SIZE 256
#define IP_COUNTER "10.59.152.222"
#define PORT_COUNTER 8082

command_t parse_cmd(const char *input)
{
    command_t cmd = {0};
    int success = 0;
    
    if (input != NULL && strlen(input) != 0)
    {
        char copy[BUF_SIZE];
        strncpy(copy, input, sizeof(copy) - 1);
        copy[sizeof(copy) - 1] = '\0';

        char *tokens[10];
        int token_count = 0;
        
        char *token = strtok(copy, " ");
        while (token && token_count < 10)
        {
            tokens[token_count] = token;
            token_count++;
            token = strtok(NULL, " ");
        }

        if (token_count >= 3)
        {
            strncpy(cmd.cmd, tokens[0], sizeof(cmd.cmd) - 1);
            char *type_str = tokens[1];
            cmd.args_count = strlen(type_str);

            cmd.args = malloc(cmd.args_count * sizeof(arg_t));
            if (cmd.args != NULL)
            {
                memset(cmd.args, 0, cmd.args_count * sizeof(arg_t));
                success = 1;
                
                for (int i = 0; i < cmd.args_count; i++)
                {
                    if (2 + i < token_count)
                    {
                        if (type_str[i] == 'i')
                        {
                            cmd.args[i].type = INT;
                            cmd.args[i].value.val_i = atoi(tokens[2 + i]);
                        }
                        else if (type_str[i] == 'f')
                        {
                            cmd.args[i].type = FLOAT;
                            cmd.args[i].value.val_f = atof(tokens[2 + i]);
                        }
                        else if (type_str[i] == 'r')
                        {
                            cmd.args[i].type = BD_KEY;
                            size_t len = strlen(tokens[2 + i]) + 1;
                            cmd.args[i].value.bd_key = malloc(len);
                            if (cmd.args[i].value.bd_key != NULL)
                                strcpy(cmd.args[i].value.bd_key, tokens[2 + i]);
                            else
                                success = 0;
                        }
                    }
                }
                
                if (success)
                {
                    int result_idx = 2 + cmd.args_count;
                    if (result_idx < token_count)
                    {
                        size_t len = strlen(tokens[result_idx]) + 1;
                        cmd.res_key = malloc(len);
                        if (cmd.res_key != NULL)
                            strcpy(cmd.res_key, tokens[result_idx]);
                        else
                            success = 0;
                    }
                }
            }
        }
    }
    
    if (!success && cmd.args != NULL)
    {
        free(cmd.args);
        cmd.args = NULL;
        cmd.args_count = 0;
    }

    return cmd;
}

int connect_to_server(const char *ip, int port)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0)
        print_sock_err();
    else
    {
        struct sockaddr_in addr = {0};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);

        if (inet_pton(AF_INET, ip, &addr.sin_addr) == 0)
        {
            print_ip_err(ip);
            close(sock);
            sock = -1;
        }
        else if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0)
        {
            print_connect_err(ip, port);
            close(sock);
            sock = -1;
        }
    }
    return sock;
}

int create_server(int port)
{
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);

    if (server_sock < 0)
        printf("[ERROR] Не удалось создать серверный сокет\n");
    else
    {
        int opt = 1;
        setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
        struct sockaddr_in addr = {0};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);

        if (bind(server_sock, (struct sockaddr*)&addr, sizeof(addr)) < 0)
        {
            printf("[ERROR] Не удалось привязать сокет к порту %d\n", port);
            close(server_sock);
            server_sock = -1;
        }
        else if (listen(server_sock, 5) < 0)
        {
            printf("[ERROR] Не удалось начать прослушивание порта %d\n", port);
            close(server_sock);
            server_sock = -1;
        }
    }
    return server_sock;
}

int send_line(int sock, const char *str)
{
    int total_sent = 0;
    int len = strlen(str);
    int res = 0;

    while (total_sent < len && res >= 0)
    {
        int sent = send(sock, str + total_sent, len - total_sent, 0);
        if (sent < 0)
            res = -1;
        else
            total_sent += sent;
    }

    if (res >= 0 && send(sock, "\n", 1, 0) < 0)
        res = -1;

    return res;
}

char *send_and_receive(int sock, const char *cmd, const char *param1, const char *param2, char *buf, int buf_size)
{
    if (sock < 0)
        strcpy(buf, "NOT CONNECTED");
    else
    {
        int wait_recv = 1;
        if (send_line(sock, cmd) < 0) 
        { 
            strcpy(buf, "SEND_ERROR"); 
            wait_recv = 0; 
        }

        if (wait_recv && param1 && send_line(sock, param1) < 0) 
        { 
            strcpy(buf, "SEND_ERROR"); 
            wait_recv = 0; 
        }

        if (wait_recv && param2 && send_line(sock, param2) < 0) 
        { 
            strcpy(buf, "SEND_ERROR"); 
            wait_recv = 0; 
        }

        if (wait_recv)
        {
            memset(buf, 0, buf_size);
            int received = 0;
            char *ptr = buf;
            int stop = 0;

            while (!stop && received < buf_size - 1)
            {
                int bytes = recv(sock, ptr, 1, 0);
                if (bytes > 0)
                {
                    if (*ptr == '\n')
                    {
                        *ptr = '\0';
                        stop = 1;
                    }
                    else
                    {
                        ptr++;
                        received++;
                    }
                }
                else
                    stop = 1;
            }
            if (received == 0)
                strcpy(buf, "NO RESPONSE");
        }
    }
    return buf;
}

char *db_send_receive(int sock, const char *cmd, const char *key, const char *value, char *buf, int buf_size)
{
    if (sock < 0) 
    {
        strcpy(buf, "NOT CONNECTED");
        return buf;
    }
    
    char send_buf[BUF_SIZE * 2];
    memset(send_buf, 0, sizeof(send_buf));
    
    if (value) 
        snprintf(send_buf, sizeof(send_buf) - 1, "%s %s %s", cmd, key, value);
    else 
        snprintf(send_buf, sizeof(send_buf) - 1, "%s %s", cmd, key);
    
    strcat(send_buf, "\n");
    
    if (send(sock, send_buf, strlen(send_buf), 0) < 0) 
    {
        strcpy(buf, "SEND_ERROR");
        return buf;
    }
    
    memset(buf, 0, buf_size);
    int received = 0;
    char *ptr = buf;
    int stop = 0;

    while (!stop && received < buf_size - 1) 
    {
        int bytes = recv(sock, ptr, 1, 0);
        if (bytes > 0) 
        {
            if (*ptr == '\n') 
            {
                *ptr = '\0';
                stop = 1;
            } 
            else 
            {
                ptr++;
                received++;
            }
        } 
        else if (bytes == 0) 
        {
            strcpy(buf, "SERVER_CLOSED");
            stop = 1;
        } 
        else 
        {
            strcpy(buf, "RECV_ERROR");
            stop = 1;
        }
    }
    
    if (received == 0 && strcmp(buf, "SERVER_CLOSED") != 0 && strcmp(buf, "RECV_ERROR") != 0) 
        strcpy(buf, "NO RESPONSE");
    
    return buf;
}

char *db_get(int db_sock, const char *key)
{
    static char buf[BUF_SIZE];
    return db_send_receive(db_sock, "get", key, NULL, buf, sizeof(buf));
}

void db_insert(int db_sock, const char *key, const char *value)
{
    char buf[BUF_SIZE];
    db_send_receive(db_sock, "insert", key, value, buf, sizeof(buf));
}

void free_command(command_t *cmd)
{
    if (cmd)
    {
        if (cmd->args)
        {
            for (int i = 0; i < cmd->args_count; i++)
                if (cmd->args[i].type == BD_KEY && cmd->args[i].value.bd_key)
                    free(cmd->args[i].value.bd_key);
            free(cmd->args);
        }
        if (cmd->res_key)
            free(cmd->res_key);
    }
}

void perform_cmd(command_t *cmd, int apu_sock, int fpu_sock, int db_sock)
{
    char param1_str[50] = {0};
    char param2_str[50] = {0};
    int success = 1;
    int i = 0;
    int has_float_from_db = 0;
    
    while (i < cmd->args_count && success)
    {
        if (cmd->args[i].type == BD_KEY)
        {
            if (cmd->args[i].value.bd_key == NULL)
                success = 0;
            else
            {
                char *db_value = db_get(db_sock, cmd->args[i].value.bd_key);
                if (strcmp(db_value, "KEY_NOT_FOUND") == 0)
                {
                    printf("[ERROR] Ключ не найден: %s\n", cmd->args[i].value.bd_key);
                    success = 0;
                }
                else
                {
                    if (strchr(db_value, '.') != NULL)
                        has_float_from_db = 1;
                    
                    if (i == 0)
                        strcpy(param1_str, db_value);
                    else if (i == 1)
                        strcpy(param2_str, db_value);
                }
            }
        }
        else if (cmd->args[i].type == INT)
        {
            if (i == 0) sprintf(param1_str, "%d", cmd->args[i].value.val_i);
            else if (i == 1) sprintf(param2_str, "%d", cmd->args[i].value.val_i);
        }
        else if (cmd->args[i].type == FLOAT)
        {
            if (i == 0) sprintf(param1_str, "%.6f", cmd->args[i].value.val_f);
            else if (i == 1) sprintf(param2_str, "%.6f", cmd->args[i].value.val_f);
        }
        i++;
    }
    
    if (success)
    {
        char target_cmd[20] = {0};
        int target_sock = apu_sock;
        int use_fpu = 0;
        i = 0;
        
        while (i < cmd->args_count && !use_fpu)
        {
            if (cmd->args[i].type == FLOAT)
                use_fpu = 1;
            i++;
        }
        
        if (has_float_from_db)
            use_fpu = 1;
        
        if (use_fpu)
        {
            target_sock = fpu_sock;
            if (strcmp(cmd->cmd, "add") == 0) 
                strcpy(target_cmd, "fadd");
            else if (strcmp(cmd->cmd, "sub") == 0) 
                strcpy(target_cmd, "fsub");
            else if (strcmp(cmd->cmd, "div") == 0) 
                strcpy(target_cmd, "fdiv");
            else if (strcmp(cmd->cmd, "sin") == 0)  
                strcpy(target_cmd, "fsin");
            else if (strcmp(cmd->cmd, "cmp") == 0) 
                strcpy(target_cmd, "fcmp");
            else 
                strcpy(target_cmd, cmd->cmd);
        }
        else
        {
            if (strcmp(cmd->cmd, "sum") == 0) 
                strcpy(target_cmd, "int_sum");
            else if (strcmp(cmd->cmd, "sub") == 0) 
                strcpy(target_cmd, "int_sub");
            else if (strcmp(cmd->cmd, "mul") == 0) 
                strcpy(target_cmd, "int_mult");
            else if (strcmp(cmd->cmd, "div") == 0) 
                strcpy(target_cmd, "int_div");
            else if (strcmp(cmd->cmd, "cmp") == 0) 
                strcpy(target_cmd, "int_comp");
            else if (strcmp(cmd->cmd, "neg") == 0) 
                strcpy(target_cmd, "int_neg");
            else 
                strcpy(target_cmd, cmd->cmd);
        }
        
        char result[BUF_SIZE] = {0};
        if (cmd->args_count == 1)
            send_and_receive(target_sock, target_cmd, param1_str, NULL, result, sizeof(result));
        else if (cmd->args_count == 2)
            send_and_receive(target_sock, target_cmd, param1_str, param2_str, result, sizeof(result));
        
        if (strncmp(result, "ERROR", 5) == 0 || strncmp(result, "NOT", 3) == 0)
            printf("[ERROR] %s\n", result);
        else
        {
            printf("[RESULT] %s\n", result);
            if (cmd->res_key)
                db_insert(db_sock, cmd->res_key, result);
        }
    }
}

void handle_counter_client(int client_sock, int apu_sock, int fpu_sock, int db_sock)
{
    char cmd[BUF_SIZE];
    
    while (1)  
    {
        memset(cmd, 0, sizeof(cmd));
        int bytes_received = 0;
        
        while (bytes_received < BUF_SIZE - 1)
        {
            int bytes = recv(client_sock, cmd + bytes_received, 1, 0);
            if (bytes <= 0)
            {
                close(client_sock);
                return;
            }
            if (cmd[bytes_received] == '\0' || cmd[bytes_received] == '\n')
            {
                cmd[bytes_received] = '\0';
                break;
            }
            bytes_received++;
        }

        if (bytes_received == 0)
            continue;

        if (strcmp(cmd, "exit") == 0)
        {
            send(client_sock, "GOODBYE\0", 9, 0);
            close(client_sock);
            return;
        }

        int counter_sock = connect_to_server(IP_COUNTER, PORT_COUNTER);
        if (counter_sock >= 0)
        {
            int sent = send(counter_sock, cmd, strlen(cmd), 0);
            if (sent >= 0)
            {
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
                
                char response_with_null[BUF_SIZE + 1];
                int len = snprintf(response_with_null, sizeof(response_with_null), "%s", response);
                response_with_null[len] = '\0'; 
                send(client_sock, response_with_null, len + 1, 0); 
            }
            else
                send(client_sock, "ERROR\0", 6, 0);

            close(counter_sock);
        }
        else
        {
            send(client_sock, "COUNTER_SERVER_ERROR\0", 21, 0);
        }
        
        command_t parsed_cmd = parse_cmd(cmd);
        if (parsed_cmd.args_count > 0)
        {
            if (strncmp(cmd, "JMP", 3) != 0)
                perform_cmd(&parsed_cmd, apu_sock, fpu_sock, db_sock);

            free_command(&parsed_cmd);
        }
    }
}