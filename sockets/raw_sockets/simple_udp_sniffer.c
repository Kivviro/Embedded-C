/*
Протокол IPPROTO_RAW подразумевает включение IP_HDRINCL и позволяет
отправлять любой IP-протокол, указанный в переданном заголовке.
Прием всех IP-протоколов через IPPROTO_RAW невозможен
при использовании raw сокетов.

IP_MTU_DISCOVER - отключает проверку MTU, что не рекомендуют в рамках оптимизации
*/

#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/ip.h>
#include <netinet/udp.h>

int main()
{
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);

    char buf[65536];

    while(1)
    {
        int size = recv(sock, buf, sizeof(buf), 0);

        if (size < 0)
            continue;

        struct iphdr *ip = (struct iphdr *)buf;
        struct udphdr *udp = (struct udphdr *)(buf + ip->ihl * 4);

        struct in_addr src, dst;
        src.s_addr = ip->saddr;
        dst.s_addr = ip->daddr;

        printf("UDP |");
        if (ntohs(udp->dest) == 53 || ntohs(udp->source) == 53)
            printf(" DNS |");

        printf(" %s:%d -> %s:%d | packet=%d bytes | udp_len=%d\n", 
            inet_ntoa(src), ntohs(udp->source), 
            inet_ntoa(dst), ntohs(udp->dest), 
            size, ntohs(udp->len));

    }

    close(sock);
}