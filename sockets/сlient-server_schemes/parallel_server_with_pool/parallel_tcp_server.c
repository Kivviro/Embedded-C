#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/select.h>
#include <time.h>

#define LISTENER_PORT 8080
#define BACKLOG 5
#define MAX_SERVICES 5

typedef struct {
    int port;
    int busy;
    int pipe_fd;
} ServiceInfo;

static void get_time(char *buf, size_t size)
{
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    strftime(buf, size, "%Y-%m-%d %H:%M:%S\n", &tm);
}

void service_process(int service_id, int server_fd, int notify_fd)
{
    listen(server_fd, 1);

    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    getsockname(server_fd, (struct sockaddr*)&addr, &len);

    int service_port = ntohs(addr.sin_port);

    write(notify_fd, "ready", 5);

    printf("Service_%d started on port %d\n", service_id, service_port);

    while (1)
    {
        int client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0) 
            continue;

        write(notify_fd, "busy", 4);

        char buf[1024];
        ssize_t bytes = recv(client_fd, buf, sizeof(buf) - 1, 0);

        if (bytes > 0)
        {
            buf[bytes] = '\0';

            char time_buf[128];
            get_time(time_buf, sizeof(time_buf));

            send(client_fd, time_buf, strlen(time_buf), 0);
        }

        close(client_fd);
        write(notify_fd, "free", 4);
    }
}

int main()
{
    signal(SIGCHLD, SIG_IGN);

    int listener_fd = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(listener_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(LISTENER_PORT);

    bind(listener_fd, (struct sockaddr*)&addr, sizeof(addr));

    listen(listener_fd, BACKLOG);

    printf("Listener started on port %d\n", LISTENER_PORT);

    ServiceInfo service_objs[MAX_SERVICES];

    for (int i = 0; i < MAX_SERVICES; i++)
    {
        int pipefd[2];
        pipe(pipefd);

        int service_fd = socket(AF_INET, SOCK_STREAM, 0);

        struct sockaddr_in saddr;
        memset(&saddr, 0, sizeof(saddr));

        saddr.sin_family = AF_INET;
        saddr.sin_addr.s_addr = INADDR_ANY;
        saddr.sin_port = 0;

        bind(service_fd, (struct sockaddr*)&saddr, sizeof(saddr));

        socklen_t len = sizeof(saddr);
        getsockname(service_fd, (struct sockaddr*)&saddr, &len);

        int service_port = ntohs(saddr.sin_port);

        pid_t pid = fork();

        if (pid == 0)
        {
            close(listener_fd);
            close(pipefd[0]);

            service_process(i, service_fd, pipefd[1]);
            exit(0);
        }

        close(pipefd[1]);

        service_objs[i].port = service_port;
        service_objs[i].busy = 0;
        service_objs[i].pipe_fd = pipefd[0];

        printf("Init service %d on port %d\n", i, service_port);
    }

    fd_set readfds;

    while (1)
    {
        FD_ZERO(&readfds);

        FD_SET(listener_fd, &readfds);
        int maxfd = listener_fd;

        for (int i = 0; i < MAX_SERVICES; i++)
        {
            FD_SET(service_objs[i].pipe_fd, &readfds);
            if (service_objs[i].pipe_fd > maxfd)
                maxfd = service_objs[i].pipe_fd;
        }

        select(maxfd + 1, &readfds, NULL, NULL, NULL);

        for (int i = 0; i < MAX_SERVICES; i++)
        {
            if (FD_ISSET(service_objs[i].pipe_fd, &readfds))
            {
                char buf[6] = {0};
                read(service_objs[i].pipe_fd, buf, 5);

                if (strncmp(buf, "busy", 4) == 0)
                    service_objs[i].busy = 1;
                else if (strncmp(buf, "free", 4) == 0)
                    service_objs[i].busy = 0;
                else if (strncmp(buf, "ready", 5) == 0)
                    service_objs[i].busy = 0;

                printf("Service %d status updated\n", i);
            }
        }

        if (FD_ISSET(listener_fd, &readfds))
        {
            int client_fd = accept(listener_fd, NULL, NULL);

            int found = -1;

            for (int i = 0; i < MAX_SERVICES; i++)
            {
                if (!service_objs[i].busy)
                {
                    found = i;
                    break;
                }
            }

            if (found == -1)
            {
                int msg = -1;
                msg = htonl(msg);
                send(client_fd, &msg, sizeof(msg), 0);
                close(client_fd);
                continue;
            }

            int port_net = htonl(service_objs[found].port);
            send(client_fd, &port_net, sizeof(port_net), 0);

            service_objs[found].busy = 1;

            close(client_fd);
        }
    }

    close(listener_fd);
    return 0;
}