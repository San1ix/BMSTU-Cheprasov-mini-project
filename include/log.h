#ifndef LOG_H__
#define LOG_H__

#include <stdio.h>
#include "data.h"

void print_sock_err(void);
void print_connect_err(const char *ip, int port);
void print_ip_err(const char *ip);
void print_help(void);
void print_connect_success(int port);
void print_err_all_connect(void);

#endif