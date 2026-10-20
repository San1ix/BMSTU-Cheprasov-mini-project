#ifndef DATA_H__
#define DATA_H__

typedef enum 
{
    SUCCESS_CODE,
    ERR_IO,
    ERR_MEM,
    ERR_RANGE,
    ERR_NOT_FOUND
} status_t;

typedef enum
{
    INT,
    FLOAT,
    BD_KEY,
} param_type;

typedef struct
{
    param_type type;
    union
    {
        int val_i;
        float val_f;
        char *bd_key;
    } value;
} arg_t;

typedef struct
{
    char cmd[10];
    arg_t *args; 
    int args_count;
    char *res_key;
} command_t;

#endif