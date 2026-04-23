// client
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int main()
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);

    char buf[1024];

    sendto(sock, "Hello!\n", 7, 0, (struct sockaddr*)&server, sizeof(server));

    int n = recvfrom(sock, buf, sizeof(buf), 0, NULL, NULL);

    buf[n] = 0;
    printf("Server: %s", buf);

    close(sock);
}