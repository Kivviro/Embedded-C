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
    addr.sin_addr.s_addr = INADDR_ANY;

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

    close(client);
    close(sock);
}