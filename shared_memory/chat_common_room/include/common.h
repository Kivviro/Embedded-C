#ifndef COMMON_H
#define COMMON_H

#include <pthread.h>

#define SHM_NAME "/chat_shm"

#define MAX_MSG_SIZE 1024
#define CLIENT_NAME_LEN 64
#define MAX_MESSAGES_TOTAL 2048
#define MAX_CLIENTS 256

typedef struct 
{
    char sender[CLIENT_NAME_LEN];
    char text[MAX_MSG_SIZE];
} message_t;

typedef struct 
{
    int pid;
} client_info_t;

typedef struct 
{
    pthread_mutex_t lock;
    pthread_cond_t cond;

    message_t messages[MAX_MESSAGES_TOTAL];
    int msg_count;

    client_info_t clients[MAX_CLIENTS];
    int client_count;
} shm_chat_t;

#endif