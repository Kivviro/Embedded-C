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

    struct ip_mreqn iface = {0};

    iface.imr_ifindex = 0;

    setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF, &iface, sizeof(iface));

    struct sockaddr_in dst = {0};
    dst.sin_family = AF_INET;
    dst.sin_port = htons(PORT);
    dst.sin_addr.s_addr = inet_addr(GROUP);

    struct sockaddr_in local;

    local.sin_family = AF_INET;
    local.sin_port = htons(8080);
    local.sin_addr.s_addr = htonl(INADDR_ANY);

    bind(sock, (struct sockaddr*)&local, sizeof(local));

    printf("Sending to %s:%d via %s\n", GROUP, PORT, "Any");

    for(int counter = 0; counter < 10; counter++) 
    {
        char msg[256];

        snprintf(msg, sizeof(msg), "This is string No%d", counter);

        if (!(sendto(sock, msg, strlen(msg), 0, (struct sockaddr*)&dst, 
        sizeof(dst)) < 0)) 
        {
            printf("Send: %s\n", msg);
        }

        sleep(1);
    }

    close(sock);
    return 0;
}