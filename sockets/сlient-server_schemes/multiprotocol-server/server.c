#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <fcntl.h>

#define PORT 8080
#define MAX_EVENTS 10

static int make_socket_non_blocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return -1;

    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static void get_time(char *buf, size_t size)
{
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    strftime(buf, size, "%Y-%m-%d %H:%M:%S\n", &tm);
}

int main()
{
    int tcp_sock, udp_sock, epfd;
    struct sockaddr_in addr;

    tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
    udp_sock = socket(AF_INET, SOCK_DGRAM, 0);

    int opt = 1;
    setsockopt(tcp_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    setsockopt(udp_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(tcp_sock, (struct sockaddr*)&addr, sizeof(addr));
    bind(udp_sock, (struct sockaddr*)&addr, sizeof(addr));


    listen(tcp_sock, SOMAXCONN);

    make_socket_non_blocking(tcp_sock);
    make_socket_non_blocking(udp_sock);

    epfd = epoll_create1(0);

    struct epoll_event ev, events[MAX_EVENTS];

    ev.events = EPOLLIN;
    ev.data.fd = tcp_sock;
    epoll_ctl(epfd, EPOLL_CTL_ADD, tcp_sock, &ev);

    ev.data.fd = udp_sock;
    epoll_ctl(epfd, EPOLL_CTL_ADD, udp_sock, &ev);

    printf("Server started on port %d...\n", PORT);

    while (1) 
    {
        int nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);

        for (int i = 0; i < nfds; i++) 
        {
            int fd = events[i].data.fd;

            if (fd == tcp_sock) 
            {
                struct sockaddr_in client_addr;
                socklen_t len = sizeof(client_addr);

                int client_fd = accept(tcp_sock, (struct sockaddr*)&client_addr, &len);

                char timebuf[64];
                get_time(timebuf, sizeof(timebuf));

                write(client_fd, timebuf, strlen(timebuf));
                close(client_fd);
            }
            else if (fd == udp_sock) 
            {
                char buf[1];
                struct sockaddr_in client_addr;
                socklen_t len = sizeof(client_addr);

                recvfrom(udp_sock, buf, sizeof(buf), 0,
                         (struct sockaddr*)&client_addr, &len);

                char timebuf[64];
                get_time(timebuf, sizeof(timebuf));

                sendto(udp_sock, timebuf, strlen(timebuf), 0,
                       (struct sockaddr*)&client_addr, len);
            }
        }
    }

    close(tcp_sock);
    close(udp_sock);
    close(epfd);
    return 0;
}