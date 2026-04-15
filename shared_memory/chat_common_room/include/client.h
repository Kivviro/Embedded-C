#ifndef CLIENT_H
#define CLIENT_H

#include <pthread.h>
#include <sys/types.h>
#include "common.h"

#define MAX_DISPLAY_LINES 1000

typedef struct 
{
    char text[MAX_MSG_SIZE + CLIENT_NAME_LEN + 16];
} display_line_t;

typedef struct 
{
    pid_t pid;

    int shm_fd;
    shm_chat_t *shm;

    int last_msg;

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

int client_core_start(client_state_t *st, pthread_t *reader_tid);
int client_send_text(client_state_t *st, const char *text);
void client_core_stop(client_state_t *st, pthread_t reader_tid);

#endif