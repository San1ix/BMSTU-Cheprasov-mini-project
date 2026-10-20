#include "log.h"

void print_connect_err(const char *ip, int port)
{
    printf("[ERROR] Не удалось подключиться к %s:%d\n", ip, port);
}

void print_sock_err(void)
{
    printf("[ERROR] Не удалось создать сокет\n");
}

void print_ip_err(const char *ip)
{
    printf("[ERROR] Неверный IP адрес: %s\n", ip);
}

void print_help(void)
{
    printf("Доступные команды: sum, sub, mult, div, cmp, neg\n");
}

void print_connect_success(int port)
{
    printf("[INFO] Успешно подключено к порту %d\n", port);
}

void print_err_all_connect(void)
{
    printf("[ERROR] Не удалось подключиться ко всем серверам\n");
}