#define _POSIX_C_SOURCE 200809L
#include <mqueue.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>

#define SERVER_QUEUE_NAME "/server_queue"
#define MAX_MSG_SIZE 1024
#define MAX_MESSAGES 10
#define CLIENT_NAME_LEN 64
#define MAX_HISTORY 256

// struct for storage messages in history
typedef struct 
{
    char sender[CLIENT_NAME_LEN];
    char text[MAX_MSG_SIZE];
} hist_entry_t;

// history array
static hist_entry_t history[MAX_HISTORY];
static int history_start = 0;
static int history_count = 0;
static pthread_mutex_t history_lock = PTHREAD_MUTEX_INITIALIZER;

// Client struct: storage queue name and descryptor
typedef struct client 
{
    char qname[CLIENT_NAME_LEN];
    mqd_t mqd;
    struct client *next;
} client_t;

static client_t *clients_head = NULL;
static pthread_mutex_t clients_lock = PTHREAD_MUTEX_INITIALIZER;

// server's queue MQ
static mqd_t server_mqd = (mqd_t)-1;
static struct mq_attr server_attr;

// stop flag
static volatile sig_atomic_t stop_requested = 0;

// add message, del older
static void push_history(const char *sender, const char *text) 
{
    pthread_mutex_lock(&history_lock);
    int idx = (history_start + history_count) % MAX_HISTORY;

    if (history_count == MAX_HISTORY) 
    {
        history_start = (history_start + 1) % MAX_HISTORY;
        idx = (history_start + history_count - 1) % MAX_HISTORY;
    } 
    else 
    {
        history_count++;
    }

    strncpy(history[idx].sender, sender, CLIENT_NAME_LEN-1);
    history[idx].sender[CLIENT_NAME_LEN-1] = '\0';
    strncpy(history[idx].text, text, MAX_MSG_SIZE-1);
    history[idx].text[MAX_MSG_SIZE-1] = '\0';
    pthread_mutex_unlock(&history_lock);
}

// broadcast sending messages to all clients
static void broadcast_message(const char *label, const char *text) 
{
    char buf[MAX_MSG_SIZE + CLIENT_NAME_LEN + 8];
    snprintf(buf, sizeof(buf), "%s:%s", label, text);

    pthread_mutex_lock(&clients_lock);
    client_t *c = clients_head;
    while (c) 
    {
        if (c->mqd != (mqd_t)-1) 
        {
            if (mq_send(c->mqd, buf, strlen(buf) + 1, 0) == -1) 
            {
                fprintf(stderr, "mq_send to %s failed: %s\n", c->qname, strerror(errno));
            }
        }
        c = c->next;
    }
    pthread_mutex_unlock(&clients_lock);
}

// send history of messages to client :)
static void send_history_to_client(mqd_t client_mqd) 
{
    pthread_mutex_lock(&history_lock);
    for (int i = 0; i < history_count; ++i) 
    {
        int idx = (history_start + i) % MAX_HISTORY;
        char buf[MAX_MSG_SIZE + CLIENT_NAME_LEN + 8];
        snprintf(buf, sizeof(buf), "%s:%s", history[idx].sender, history[idx].text);
        if (mq_send(client_mqd, buf, strlen(buf) + 1, 0) == -1) 
        {
            fprintf(stderr, "send_history_to_client: mq_send failed: %s\n", strerror(errno));
            break;
        }
    }
    pthread_mutex_unlock(&history_lock);
}

static client_t *find_client_by_name(const char *qname) 
{
    client_t *c = clients_head;
    while (c) 
    {
        if (strcmp(c->qname, qname) == 0) 
            return c;
        c = c->next;
    }
    return NULL;
}

// add client
static void add_or_update_client(const char *qname, mqd_t mqd) 
{
    pthread_mutex_lock(&clients_lock);
    client_t *c = find_client_by_name(qname);
    if (!c) 
    {
        c = calloc(1, sizeof(client_t));
        strncpy(c->qname, qname, CLIENT_NAME_LEN-1);
        c->mqd = mqd;
        c->next = clients_head;
        clients_head = c;
    } 
    else 
    {
        c->mqd = mqd;
    }
    pthread_mutex_unlock(&clients_lock);
}

// remove client
static void remove_client_by_name(const char *qname) 
{
    pthread_mutex_lock(&clients_lock);
    client_t **pp = &clients_head;
    while (*pp) 
    {
        client_t *cur = *pp;
        if (strcmp(cur->qname, qname) == 0) 
        {
            *pp = cur->next;
            if (cur->mqd != (mqd_t)-1) mq_close(cur->mqd);
            free(cur);
            break;
        }
        pp = &cur->next;
    }
    pthread_mutex_unlock(&clients_lock);
}

// clearing queue descriptors
static void cleanup(void) 
{
    pthread_mutex_lock(&clients_lock);
    client_t *c = clients_head;
    while (c) 
    {
        if (c->mqd != (mqd_t)-1) 
            mq_close(c->mqd);
        client_t *n = c->next;
        free(c);
        c = n;
    }
    clients_head = NULL;
    pthread_mutex_unlock(&clients_lock);

    if (server_mqd != (mqd_t)-1) 
    {
        mq_close(server_mqd);
        mq_unlink(SERVER_QUEUE_NAME);
        server_mqd = (mqd_t)-1;
    }
}

static void sigint_handler(int signo) 
{
    (void)signo;
    stop_requested = 1;
}

int main(void) 
{
    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    server_attr.mq_flags = 0;
    server_attr.mq_maxmsg = MAX_MESSAGES;
    server_attr.mq_msgsize = MAX_MSG_SIZE;
    server_attr.mq_curmsgs = 0;

    server_mqd = mq_open(SERVER_QUEUE_NAME, O_CREAT | O_RDONLY, 0666, &server_attr);
    if (server_mqd == (mqd_t) - 1) 
    {
        exit(EXIT_FAILURE);
    }

    printf("Server started, queue: %s\n", SERVER_QUEUE_NAME);

    char buf[MAX_MSG_SIZE];
    unsigned int prio;

    while (!stop_requested) 
    {
        ssize_t r = mq_receive(server_mqd, buf, MAX_MSG_SIZE, &prio);
        if (r >= 0) 
        {
            buf[r] = '\0';

            if (strncmp(buf, "JOIN:", 5) == 0) 
            {
                char pid[32];
                snprintf(pid, sizeof(pid), "%s", buf + 5);
                char client_qname[CLIENT_NAME_LEN];
                snprintf(client_qname, sizeof(client_qname), "/client_%s", pid);
                mqd_t client_mqd = mq_open(client_qname, O_WRONLY);
                if (client_mqd == (mqd_t)-1) 
                {
                    fprintf(stderr, "JOIN: cannot open client queue %s: %s\n", client_qname, strerror(errno));
                    continue;
                }

                add_or_update_client(client_qname, client_mqd);

                char srv_msg[MAX_MSG_SIZE];
                snprintf(srv_msg, sizeof(srv_msg), "User %s joined", pid);
                push_history("SERVER", srv_msg);
                broadcast_message("SERVER", srv_msg);

                send_history_to_client(client_mqd);
            } 
            else if (strncmp(buf, "MSG:", 4) == 0) 
            {
                char *p = buf + 4;
                char *colon = strchr(p, ':');
                if (!colon) 
                    continue;
                *colon = '\0';

                char pid[32];
                strncpy(pid, p, sizeof(pid)-1);
                pid[sizeof(pid)-1] = '\0';

                char *text = colon + 1;
                char label[CLIENT_NAME_LEN];
                snprintf(label, sizeof(label), "Client_%s", pid);

                push_history(label, text);

                char outbuf[MAX_MSG_SIZE + CLIENT_NAME_LEN + 8];
                snprintf(outbuf, sizeof(outbuf), "%s:%s", label, text);
                broadcast_message(label, text);
            } 
            else if (strncmp(buf, "LEAVE:", 6) == 0) 
            {
                char pid[32];
                snprintf(pid, sizeof(pid), "%s", buf + 6);

                char client_qname[CLIENT_NAME_LEN];
                snprintf(client_qname, sizeof(client_qname), "/client_%s", pid);

                remove_client_by_name(client_qname);

                char srv_msg[MAX_MSG_SIZE];
                snprintf(srv_msg, sizeof(srv_msg), "User %s left", pid);
                push_history("SERVER", srv_msg);
                broadcast_message("SERVER", srv_msg);
            } 
            else 
            {
                fprintf(stderr, "Unknown message on server queue: %s\n", buf);
            }
        } 
        else 
        {
            if (errno == EINTR) 
            {
                continue;
            }
        }
    }

    printf("Server shutting down...\n");
    cleanup();
    return 0;
}