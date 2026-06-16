#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>

int main()
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    connect(sock, (struct sockaddr*)&addr, sizeof(addr));

    char buf[1024];

    send(sock, "Hello!\n", 7, 0);

    int n = recv(sock, buf, sizeof(buf)-1, 0);
    
    buf[n] = 0;
    printf("Server: %s", buf);

    close(sock);
}