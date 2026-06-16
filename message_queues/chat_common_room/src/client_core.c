#define _POSIX_C_SOURCE 200809L
#include "client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

// Thread: retrieving messages from its own queue and adding them to the UI buffer
static void *mq_reader_thread(void *arg) 
{
    client_state_t *st = (client_state_t*)arg;
    char buf[MAX_MSG_SIZE + CLIENT_NAME_LEN + 16];
    unsigned int prio;

    while (!st->stop_requested) 
    {
        ssize_t r = mq_receive(st->client_mqd, buf, sizeof(buf), &prio);
        if (r >= 0) 
        {
            buf[r] = '\0';
            pthread_mutex_lock(&st->lines_lock);
            if (st->lines_count < MAX_DISPLAY_LINES) 
            {
                strncpy(st->lines[st->lines_count].text, buf, sizeof(st->lines[st->lines_count].text)-1);
                st->lines[st->lines_count].text[sizeof(st->lines[st->lines_count].text)-1] = '\0';
                st->lines_count++;
            } 
            else 
            {
                memmove(&st->lines[0], &st->lines[1], sizeof(display_line_t)*(MAX_DISPLAY_LINES-1));
                strncpy(st->lines[MAX_DISPLAY_LINES-1].text, buf, sizeof(st->lines[0].text)-1);
                st->lines[MAX_DISPLAY_LINES-1].text[sizeof(st->lines[0].text)-1] = '\0';
            }
            pthread_mutex_unlock(&st->lines_lock);

            if (strncmp(buf, "[SERVER]:User ", 12) == 0) 
            {
                char *p = buf + 12;
                char pidstr[32];
                int i = 0;
                while (*p && *p != ' ' && i < (int)sizeof(pidstr)-1) 
                    pidstr[i++] = *p++;
                pidstr[i] = '\0';
                if (strstr(buf, "joined")) 
                {
                    pthread_mutex_lock(&st->clients_lock);

                    int exists = 0;
                    for (int k=0;k<st->clients_count;k++)
                    {
                        if (strcmp(st->clients[k], (char[]){'/', 'c','l','i','e','n','t','_','\0'})==0) { /* never */ }
                    }

                    char cname[CLIENT_NAME_LEN];
                    snprintf(cname, sizeof(cname), "/client_%s", pidstr);
                    exists = 0;
                    for (int k=0;k<st->clients_count;k++) if (strcmp(st->clients[k], cname)==0) 
                    { 
                        exists = 1; break; 
                    }
                    if (!exists && st->clients_count < MAX_CLIENTS) 
                    {
                        strncpy(st->clients[st->clients_count], cname, CLIENT_NAME_LEN-1);
                        st->clients[st->clients_count][CLIENT_NAME_LEN-1] = '\0';
                        st->clients_count++;
                    }
                    pthread_mutex_unlock(&st->clients_lock);
                } 
                else if (strstr(buf, "left")) 
                {
                    char cname[CLIENT_NAME_LEN];
                    snprintf(cname, sizeof(cname), "/client_%s", pidstr);
                    pthread_mutex_lock(&st->clients_lock);
                    int pos = -1;
                    for (int k=0;k<st->clients_count;k++) if (strcmp(st->clients[k], cname)==0) 
                    { 
                        pos = k; 
                        break; 
                    }
                    if (pos != -1) 
                    {
                        for (int k=pos;k<st->clients_count-1;k++) 
                            strcpy(st->clients[k], st->clients[k+1]);
                        st->clients_count--;
                    }
                    pthread_mutex_unlock(&st->clients_lock);
                }
            }

        } 
        else 
        {
            if (errno == EINTR) 
                continue;
            sleep(1);
        }
    }
    return NULL;
}

// state init
void client_state_init(client_state_t *st) 
{
    memset(st, 0, sizeof(*st));
    st->pid = getpid();
    snprintf(st->qname, sizeof(st->qname), "/client_%d", (int)st->pid);
    pthread_mutex_init(&st->lines_lock, NULL);
    pthread_mutex_init(&st->clients_lock, NULL);
    st->stop_requested = 0;
}

// Resource release (does not close the message queue, etc.)
void client_state_destroy(client_state_t *st) 
{
    pthread_mutex_destroy(&st->lines_lock);
    pthread_mutex_destroy(&st->clients_lock);
}

int client_core_start(client_state_t *st, pthread_t *reader_tid) 
{
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = MAX_MESSAGES;
    attr.mq_msgsize = MAX_MSG_SIZE;
    attr.mq_curmsgs = 0;

    st->client_mqd = mq_open(st->qname, O_CREAT | O_RDONLY, 0666, &attr);
    if (st->client_mqd == (mqd_t)-1) 
    {
        perror("mq_open(client)");
        return -1;
    }

    st->server_mqd = mq_open(SERVER_QUEUE_NAME, O_WRONLY);
    if (st->server_mqd == (mqd_t)-1) 
    {
        perror("mq_open(server)");
        mq_close(st->client_mqd);
        mq_unlink(st->qname);
        return -1;
    }

    if (pthread_create(reader_tid, NULL, mq_reader_thread, st) != 0) 
    {
        perror("pthread_create");
        mq_close(st->client_mqd);
        mq_unlink(st->qname);
        mq_close(st->server_mqd);
        return -1;
    }

    char join_buf[128];
    snprintf(join_buf, sizeof(join_buf), "JOIN:%d", (int)st->pid);
    if (mq_send(st->server_mqd, join_buf, strlen(join_buf)+1, 0) == -1) 
    {
        perror("mq_send JOIN");
    }

    return 0;
}

// send mes to server
int client_send_text(client_state_t *st, const char *text) 
{
    char out[MAX_MSG_SIZE + 64];
    snprintf(out, sizeof(out), "MSG:%d:%s", (int)st->pid, text);
    if (mq_send(st->server_mqd, out, strlen(out)+1, 0) == -1) 
    {
        perror("mq_send MSG");
        return -1;
    }
    return 0;
}

// send LEAVE and close queue
void client_core_stop(client_state_t *st, pthread_t reader_tid) 
{
    st->stop_requested = 1;

    char leave_buf[128];
    snprintf(leave_buf, sizeof(leave_buf), "LEAVE:%d", (int)st->pid);
    mq_send(st->server_mqd, leave_buf, strlen(leave_buf)+1, 0);


    if (st->client_mqd != (mqd_t)-1) 
    {
        mq_close(st->client_mqd);
    }
    pthread_join(reader_tid, NULL);

    if (st->server_mqd != (mqd_t)-1) 
        mq_close(st->server_mqd);
    mq_unlink(st->qname);
}