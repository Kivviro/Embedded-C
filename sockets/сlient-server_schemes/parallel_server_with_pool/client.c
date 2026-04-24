#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define LISTENER_PORT 8080

int connect_to_port(int port)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    connect(sock, (struct sockaddr*)&addr, sizeof(addr));

    return sock;
}

int main()
{
    int listener_fd = connect_to_port(LISTENER_PORT);

    int port_net;

    ssize_t r = recv(listener_fd, &port_net, sizeof(port_net), MSG_WAITALL);
    if (r != sizeof(port_net))
    {
        printf("Failed to read service port\n");
        return 1;
    }

    close(listener_fd);

    int service_port = ntohl(port_net);

    if (service_port == -1)
    {
        printf("No free services\n");
        return 1;
    }

    printf("Received service port: %d\n", service_port);

    int service_fd = connect_to_port(service_port);

    char *request = "time\n";
    send(service_fd, request, strlen(request), 0);

    char buf[1024];
    ssize_t bytes = recv(service_fd, buf, sizeof(buf) - 1, 0);

    if (bytes > 0)
    {
        buf[bytes] = '\0';
        printf("Server time: %s", buf);
    }

    close(service_fd);
    return 0;
}