#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int main()
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in addr = {0};

    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(sock, (struct sockaddr*)&addr, sizeof(addr));

    for(int i = 0; i < 10; i++)
    {
        char buf[1024];
        struct sockaddr_in src;
        socklen_t srclen = sizeof(src);

        ssize_t n = recvfrom(sock, buf, sizeof(buf) - 1, 0, 
        (struct sockaddr*)&src, &srclen);

        if(n < 0)
            continue;

        buf[n] = '\0';

        printf("From %s:%d -> %s\n", inet_ntoa(src.sin_addr), 
        ntohs(src.sin_port), buf);
    }

    close(sock);
    return 0;
}