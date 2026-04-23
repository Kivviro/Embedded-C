// server
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int main()
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(sock, (struct sockaadr*)&addr, sizeof(addr));
    
    char buf[1024];
    struct sockaddr_in client;
    socklen_t len = sizeof(client);

    while(1)
    {
        int n = recvfrom(sock, buf, sizeof(buf), 0, 
        (struct sockaddr*)&client, &len);

        buf[n] = 0;
        printf("Client: %s", buf);

        sendto(sock, "Hi!\n", 4, 0, (struct sockaddr*)&client, len);
    }

    close(sock);
}