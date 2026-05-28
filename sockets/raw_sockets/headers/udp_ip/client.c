#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/udp.h>
#include <netinet/ip.h>


int main()
{
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);

    int one = 1;
    setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one));

    struct sockaddr_in server = {0};
    server.sin_family = AF_INET;
    server.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);

    char data[] = "Hello udp and ip!";
    char packet[1500];

    struct iphdr *ip = (struct iphdr*)packet;
    struct udphdr *udp = (struct udphdr*)(packet + sizeof(struct iphdr));
    char *payload = packet + sizeof(struct iphdr) + sizeof(struct udphdr);

    strcpy(payload, data);

    udp->source = htons(12345);
    udp->dest = htons(8080);
    udp->len = htons(sizeof(struct udphdr) + strlen(data));
    udp->check = 0;

    ip->ihl = 5;
    ip->version = 4;
    ip->tos = 0;
    ip->tot_len = htons(sizeof(struct iphdr) + sizeof(struct udphdr) + 
        strlen(data));
    ip->id = htons(1234);
    ip->frag_off = 0;
    ip->ttl = 64;
    ip->protocol = IPPROTO_UDP;
    ip->check = 0;
    ip->saddr = inet_addr("127.0.0.1");
    ip->daddr = inet_addr("127.0.0.1");

    int packet_size = sizeof(struct iphdr) + sizeof(struct udphdr) 
        + strlen(data);

    sendto(sock, packet, packet_size, 0, 
        (struct sockaddr*)&server, sizeof(server));

    char buf[1500];

    while (1)
    {
        int n = recvfrom(sock, buf, sizeof(buf), 0, NULL, NULL);

        if (n <= 0)
            continue;

        struct iphdr *ip = (struct iphdr*)buf;
        int ip_header_len = ip->ihl * 4;

        struct udphdr *udp = (struct udphdr*)(buf + ip_header_len);

        if (ntohs(udp->dest) != 12345)
            continue;

        if (ntohs(udp->source) != 8080)
            continue;

        int udp_len = ntohs(udp->len);
        int payload_len = udp_len - sizeof(struct udphdr);

        char *payload = buf + ip_header_len + sizeof(struct udphdr);

        printf("Server: %.*s\n", payload_len, payload);

        break;
    }

    close(sock);
}