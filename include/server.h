#ifndef SERVER_H__
#define SERVER_H__

#include "data.h"

command_t parse_cmd(const char *input);

int connect_to_server(const char *ip, int port);
int create_server(int port);

int send_line(int sock, const char *str);
char *send_and_receive(int sock, const char *cmd, const char *param1, const char *param2, char *buf, int buf_size);

char *db_send_receive(int sock, const char *cmd, const char *key, const char *value, char *buf, int buf_size);
char *db_get(int db_sock, const char *key);
void db_insert(int db_sock, const char *key, const char *value);

void free_command(command_t *cmd);
void perform_cmd(command_t *cmd, int apu_sock, int fpu_sock, int db_sock);

void handle_counter_client(int client_sock, int apu_sock, int fpu_sock, int db_sock);

#endif