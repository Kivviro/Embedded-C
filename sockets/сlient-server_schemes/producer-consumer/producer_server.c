#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <time.h>

#define LISTENER_PORT 8080
#define BACKLOG 16
#define MAX_SERVICES 5
#define QUEUE_MAX 128

typedef struct {
    int head, tail, count, cap;
    int ports[QUEUE_MAX];
} shm_queue_t;

static void get_time(char *buf, size_t size)
{
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    strftime(buf, size, "%Y-%m-%d %H:%M:%S\n", &tm);
}

void enqueue(shm_queue_t *q, int port) 
{
    if (q->count == q->cap) 
        return;
    q->ports[q->tail] = port;
    q->tail = (q->tail + 1) % q->cap;
    q->count++;
}

int dequeue(shm_queue_t *q) 
{
    if (q->count == 0) 
        return -1;
    int p = q->ports[q->head];
    q->head = (q->head + 1) % q->cap;
    q->count--;

    return p;
}

void service_process(int id, shm_queue_t *q) 
{
    printf("Service_%d started\n", id);

    while (1) 
    {
        int client_port = -1;
        client_port = dequeue(q);
        if (client_port == -1) 
        {
            usleep(100000);
            continue;
        }

        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) 
            continue; 

        struct sockaddr_in caddr;
        memset(&caddr,0,sizeof(caddr));
        caddr.sin_family = AF_INET;
        caddr.sin_port = htons(client_port);
        inet_pton(AF_INET, "127.0.0.1", &caddr.sin_addr);

        if (connect(sock, (struct sockaddr*)&caddr, sizeof(caddr)) < 0) 
        {
            close(sock);
            continue;
        }

        char buf[1024];
        ssize_t n = read(sock, buf, sizeof(buf)-1);
        if (n > 0)
        {
            buf[n] = 0;

            if (strncmp(buf, "time", 4) == 0)
            {
                char timebuf[128];
                get_time(timebuf, sizeof(timebuf));
                write(sock, timebuf, strlen(timebuf));
            }
            else
            {
                const char *resp = "unknown command\n";
                write(sock, resp, strlen(resp));
            }
        }

        close(sock);
        printf("Service_%d finished client:%d\n", id, client_port);
    }

    exit(0);
}

int main() 
{
    signal(SIGCHLD, SIG_IGN);

    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;

    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in laddr;
    memset(&laddr,0,sizeof(laddr));

    laddr.sin_family = AF_INET;
    laddr.sin_addr.s_addr = INADDR_ANY;
    laddr.sin_port = htons(LISTENER_PORT);

    bind(lfd, (struct sockaddr*)&laddr, sizeof(laddr));
    listen(lfd, BACKLOG);
    printf("Listener started on port %d\n", LISTENER_PORT);

    shm_queue_t *q = mmap(NULL, sizeof(shm_queue_t), PROT_READ | PROT_WRITE,
                          MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    
    if (q == MAP_FAILED) 
        exit(1);
    q->head = q->tail = q->count = 0; q->cap = QUEUE_MAX;

    for (int i=0;i<MAX_SERVICES;i++) 
    {
        pid_t pid = fork();
        if (pid == 0) 
        {
            service_process(i, q);
            exit(0);
        }
    }

    while (1) 
    {
        int cfd = accept(lfd, NULL, NULL);
        if (cfd < 0) 
        { 
            usleep(100000); 
            continue; 
        }

        int netport;
        ssize_t r = read(cfd, &netport, sizeof(netport));

        if (r == sizeof(netport)) 
        {
            int port = ntohl(netport);
            enqueue(q, port);
            const char *ack = "queued";
            write(cfd, ack, strlen(ack));
        }

        close(cfd);
    }

    close(lfd);
    munmap(q, sizeof(*q));
    return 0;
}