#define _POSIX_C_SOURCE 200809L
#include "client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

static void *shm_reader_thread(void *arg)
{
    client_state_t *st = arg;

    while (!st->stop_requested)
    {
        pthread_mutex_lock(&st->shm->lock);

        while (st->last_msg >= st->shm->msg_count)
            pthread_cond_wait(&st->shm->cond, &st->shm->lock);

        while (st->last_msg < st->shm->msg_count)
        {
            message_t *m = &st->shm->messages[st->last_msg];

            char buf[MAX_MSG_SIZE + CLIENT_NAME_LEN + 16];
            snprintf(buf, sizeof(buf), "%s:%s", m->sender, m->text);

            if (strncmp(buf, "SERVER:User ", 12) == 0)
            {
                char pidstr[32];
                sscanf(buf, "SERVER:User %s", pidstr);

                if (strstr(buf, "joined"))
                {
                    pthread_mutex_lock(&st->clients_lock);

                    char cname[CLIENT_NAME_LEN];
                    snprintf(cname, sizeof(cname), "/client_%s", pidstr);

                    int exists = 0;
                    for (int i = 0; i < st->clients_count; i++)
                        if (strcmp(st->clients[i], cname) == 0)
                            exists = 1;

                    if (!exists && st->clients_count < MAX_CLIENTS)
                    {
                        strcpy(st->clients[st->clients_count++], cname);
                    }

                    pthread_mutex_unlock(&st->clients_lock);
                }
                else if (strstr(buf, "left"))
                {
                    pthread_mutex_lock(&st->clients_lock);

                    char cname[CLIENT_NAME_LEN];
                    snprintf(cname, sizeof(cname), "/client_%s", pidstr);

                    for (int i = 0; i < st->clients_count; i++)
                    {
                        if (strcmp(st->clients[i], cname) == 0)
                        {
                            for (int j = i; j < st->clients_count - 1; j++)
                                strcpy(st->clients[j], st->clients[j + 1]);

                            st->clients_count--;
                            break;
                        }
                    }

                    pthread_mutex_unlock(&st->clients_lock);
                }
            }

            pthread_mutex_lock(&st->lines_lock);

            if (st->lines_count < MAX_DISPLAY_LINES)
            {
                strcpy(st->lines[st->lines_count++].text, buf);
            }
            else
            {
                memmove(&st->lines[0], &st->lines[1],
                        sizeof(display_line_t) * (MAX_DISPLAY_LINES - 1));
                strcpy(st->lines[MAX_DISPLAY_LINES - 1].text, buf);
            }

            pthread_mutex_unlock(&st->lines_lock);

            st->last_msg++;
        }

        pthread_mutex_unlock(&st->shm->lock);
    }
    return NULL;
}

void client_state_init(client_state_t *st)
{
    memset(st, 0, sizeof(*st));
    st->pid = getpid();
    pthread_mutex_init(&st->lines_lock, NULL);
    pthread_mutex_init(&st->clients_lock, NULL);
}

void client_state_destroy(client_state_t *st)
{
    pthread_mutex_destroy(&st->lines_lock);
    pthread_mutex_destroy(&st->clients_lock);
}

int client_core_start(client_state_t *st, pthread_t *reader_tid)
{
    st->shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (st->shm_fd == -1)
        return -1;

    st->shm = mmap(NULL, sizeof(shm_chat_t), PROT_READ | PROT_WRITE, MAP_SHARED, 
    st->shm_fd, 0);

    if (st->shm == MAP_FAILED)
        return -1;

    pthread_mutex_lock(&st->shm->lock);

    if (st->shm->client_count < MAX_CLIENTS)
    {
        st->shm->clients[st->shm->client_count++].pid = st->pid;

        message_t *m = &st->shm->messages[st->shm->msg_count++];
        snprintf(m->sender, sizeof(m->sender), "SERVER");
        snprintf(m->text, sizeof(m->text), "User %d joined", st->pid);
    }

    pthread_cond_broadcast(&st->shm->cond);
    pthread_mutex_unlock(&st->shm->lock);

    return pthread_create(reader_tid, NULL, shm_reader_thread, st);
}

int client_send_text(client_state_t *st, const char *text)
{
    pthread_mutex_lock(&st->shm->lock);

    message_t *m = &st->shm->messages[st->shm->msg_count++];

    snprintf(m->sender, sizeof(m->sender), "Client_%d", st->pid);
    strncpy(m->text, text, MAX_MSG_SIZE - 1);

    pthread_cond_broadcast(&st->shm->cond);

    pthread_mutex_unlock(&st->shm->lock);
    return 0;
}

void client_core_stop(client_state_t *st, pthread_t reader_tid)
{
    st->stop_requested = 1;

    pthread_mutex_lock(&st->shm->lock);

    message_t *m = &st->shm->messages[st->shm->msg_count++];
    snprintf(m->sender, sizeof(m->sender), "SERVER");
    snprintf(m->text, sizeof(m->text), "User %d left", st->pid);

    pthread_cond_broadcast(&st->shm->cond);
    pthread_mutex_unlock(&st->shm->lock);

    pthread_join(reader_tid, NULL);

    munmap(st->shm, sizeof(shm_chat_t));
    close(st->shm_fd);
}