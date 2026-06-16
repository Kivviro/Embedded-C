#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>

#define PORT 8080

int main() 
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));

    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);

    sendto(sock, "x", 1, 0, (struct sockaddr*)&server, sizeof(server));

    char buf[128];
    socklen_t len = sizeof(server);

    int n = recvfrom(sock, buf, sizeof(buf)-1, 0, (struct sockaddr*)&server, &len);

    if (n > 0) 
    {
        buf[n] = '\0';
        printf("Server time: %s", buf);
    }

    close(sock);
    return 0;
}