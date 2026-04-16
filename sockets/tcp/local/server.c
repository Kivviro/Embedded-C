#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCK_PATH "/tmp/lockal_tcp_sock"

int main() 
{
    int sock = socket(AF_LOCAL, SOCK_STREAM, 0);

    struct sockaddr_un addr = {0};
    addr.sun_family = AF_LOCAL;
    strcpy(addr.sun_path, SOCK_PATH);

    unlink(SOCK_PATH);
    bind(sock, (struct sockaddr*)&addr, sizeof(addr));
    listen(sock, 5);

    int client = accept(sock, NULL, NULL);

    char buf[1024];

    while (1) 
    {
        int n = recv(client, buf, sizeof(buf)-1, 0);
        if (n <= 0)
             break;

        buf[n] = 0;
        printf("Client: %s", buf);

        send(client, "Hi!\n", 4, 0);
    }

    unlink(SOCK_PATH);
    close(client);
    close(sock);
}