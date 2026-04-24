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
    int service_port;

    read(listener_fd, &service_port, sizeof(service_port));

    close(listener_fd);

    printf("Received service port: %d.\n", service_port);

    int service_fd = connect_to_port(service_port);

    char buf[1024];

    char *request = "time\n";
    write(service_fd, request, strlen(request));

    ssize_t bytes = read(service_fd, buf, sizeof(buf) - 1);

    if (bytes > 0)
    {
        buf[bytes] = '\0';
        printf("Server time: %s", buf);
    }

    close(service_fd);
    return 0;
}