#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/udp.h>

int main()
{
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);

    struct sockaddr_in server = {0};
    server.sin_family = AF_INET;
    server.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);

    char data[] = "Hello udp!";
    char packet[1024];

    struct udphdr *udp = (struct udphdr*)packet;

    char *payload = packet + sizeof(struct udphdr);
    strcpy(payload, data);

    udp->source = htons(12345);
    udp->dest = htons(8080);
    udp->len = htons(sizeof(struct udphdr) + strlen(data));
    udp->check = 0;

    int packet_size = sizeof(struct udphdr) + strlen(data);

    sendto(sock, packet, packet_size, 0, 
        (struct sockaddr*)&server, sizeof(server));

    char buf[1024];

    while (1)
    {
        int n = recvfrom(sock, buf, sizeof(buf), 0, NULL, NULL);

        if (n <= 0)
            continue;

        unsigned char ver_ihl = buf[0];
        int ip_header_len = (ver_ihl & 0x0F) * 4;

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