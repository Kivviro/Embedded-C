#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <signal.h>
#include <time.h>

#define LISTENER_PORT 8080
#define BACKLOG 5

static void get_time(char *buf, size_t size)
{
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    strftime(buf, size, "%Y-%m-%d %H:%M:%S\n", &tm);
}

void service_process(int server_fd)
{
    listen(server_fd, 1);

    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    getsockname(server_fd, (struct sockaddr*)&addr, &len);
    int service_port = ntohs(addr.sin_port);

    printf("Service_%d started on port %d\n", service_port, service_port);

    int client_fd = accept(server_fd, NULL, NULL);

    char buf[1024];

    ssize_t bytes = read(client_fd, buf, sizeof(buf) - 1);

    if (bytes > 0)
    {
        buf[bytes] = '\0';
        printf("Service_%d received request\n", service_port);

        char time_buf[128];
        get_time(time_buf, sizeof(time_buf));

        write(client_fd, time_buf, strlen(time_buf));
    }

    printf("Service_%d is terminating...\n", service_port);

    close(client_fd);
    close(server_fd);
    exit(0);
}

int main()
{
    signal(SIGCHLD, SIG_IGN);

    int listener_fd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(LISTENER_PORT);

    bind(listener_fd, (struct sockaddr*)&addr, sizeof(addr));

    listen(listener_fd, BACKLOG);

    printf("Listener started on port %d\n", LISTENER_PORT);

    while(1)
    {
        int client_fd = accept(listener_fd, NULL, NULL);
        pid_t pid = fork();

        if (pid == 0)
        {
            close(listener_fd);

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

            write(client_fd, &service_port, sizeof(service_port));

            service_process(service_fd);
        }

        close(client_fd);        
    }

    close(listener_fd);
    return 0;
}