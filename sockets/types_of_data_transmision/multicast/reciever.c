#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>

#define GROUP "224.0.0.1"
#define PORT 8080
#define IFACE "lo"

int main()
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    int flag = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(sock, (struct sockaddr*)&addr, sizeof(addr));

    struct ip_mreqn mreq = {0};
    mreq.imr_multiaddr.s_addr = inet_addr(GROUP);
    mreq.imr_address.s_addr = htonl(INADDR_ANY);
    mreq.imr_ifindex = 0;

    setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));

    for(int i = 0; i < 10;i++)
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