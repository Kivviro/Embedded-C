#ifndef CLIENT_H
#define CLIENT_H

#include <mqueue.h>
#include <pthread.h>

#define SERVER_QUEUE_NAME "/server_queue"
#define MAX_MSG_SIZE 1024
#define MAX_MESSAGES 50
#define CLIENT_NAME_LEN 64
#define MAX_DISPLAY_LINES 1000
#define MAX_CLIENTS 256

typedef struct 
{
    char text[MAX_MSG_SIZE + CLIENT_NAME_LEN + 16];
} display_line_t;

typedef struct 
{
    pid_t pid;
    char qname[CLIENT_NAME_LEN];

    mqd_t client_mqd;
    mqd_t server_mqd;

    display_line_t lines[MAX_DISPLAY_LINES];
    int lines_count;
    pthread_mutex_t lines_lock;

    char clients[MAX_CLIENTS][CLIENT_NAME_LEN];
    int clients_count;
    pthread_mutex_t clients_lock;

    volatile int stop_requested;
} client_state_t;

void client_state_init(client_state_t *st);
void client_state_destroy(client_state_t *st);

#endif