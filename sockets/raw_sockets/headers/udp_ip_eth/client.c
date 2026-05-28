#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/udp.h>
#include <netinet/ip.h>
#include <net/ethernet.h>
#include <netpacket/packet.h>
#include <net/if.h>
#include <sys/ioctl.h>

unsigned short checksum(void *data)
{
    unsigned int csum = 0;
    unsigned short *ptr;
    unsigned int tmp;

    ptr = (unsigned short*)data;

    for (int i = 0; i < 10; i++)
    {
        csum = csum + *ptr;
        ptr++;
    }

    tmp = csum >> 16;
    csum = (csum & 0xffff) + tmp;
    csum = (csum & 0xffff) + (csum >> 16);

    return (unsigned short)(~csum);
}

int main()
{
    int sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, "enp0s3"); // src interface

    ioctl(sock, SIOCGIFHWADDR, &ifr);

    unsigned char src_mac[6];
    memcpy(src_mac, ifr.ifr_hwaddr.sa_data, 6);

    unsigned char dst_mac[6] = {0x08,0x00,0x27,0x2e,0xfa,0x08};                              

    char src_ip[] = "192.168.0.5";
    char dst_ip[] = "192.168.0.6";

    char data[] = "Hello udp, ip and ethernet!";
    char packet[1500];

    struct ethhdr *eth = (struct ethhdr*)packet;
    struct iphdr *ip = (struct iphdr*)(packet + sizeof(struct ethhdr));
    struct udphdr *udp = (struct udphdr*)(packet + 
        sizeof(struct ethhdr) + sizeof(struct iphdr));
    char *payload = packet + sizeof(struct ethhdr) + 
        sizeof(struct iphdr) + sizeof(struct udphdr);

    strcpy(payload, data);

    memcpy(eth->h_source, src_mac, 6);
    memcpy(eth->h_dest, dst_mac, 6);
    eth->h_proto = htons(ETH_P_IP);

    ip->ihl = 5;
    ip->version = 4;
    ip->tos = 0;
    ip->tot_len = htons(sizeof(struct iphdr) + sizeof(struct udphdr) + 
        strlen(data));
    ip->id = htons(1234);
    ip->frag_off = 0;
    ip->ttl = 64;
    ip->protocol = IPPROTO_UDP;
    ip->saddr = inet_addr(src_ip);
    ip->daddr = inet_addr(dst_ip);

    ip->check = 0;
    ip->check = checksum(ip);

    udp->source = htons(12345);
    udp->dest = htons(8080);
    udp->len = htons(sizeof(struct udphdr) + strlen(data));
    udp->check = 0;

    struct sockaddr_ll addr = {0};
    addr.sll_family = AF_PACKET;
    addr.sll_ifindex = if_nametoindex("enp0s3"); // dest interface
    addr.sll_halen = ETH_ALEN;
    memcpy(addr.sll_addr, dst_mac, 6);

    int packet_size = sizeof(struct ethhdr) + 
        sizeof(struct iphdr) + sizeof(struct udphdr) + 
        strlen(data);

    sendto(sock, packet, packet_size, 0, 
        (struct sockaddr*)&addr, sizeof(addr));

    char buf[1500];

    while (1)
    {
        int n = recvfrom(sock, buf, sizeof(buf), 0, NULL, NULL);

        if (n <= 0)
            continue;

        struct ethhdr *eth = (struct ethhdr*)buf;

        if (ntohs(eth->h_proto) != ETH_P_IP)
            continue;
        
        struct iphdr *ip = (struct iphdr*)(buf + sizeof(struct ethhdr));
        int ip_header_len = ip->ihl * 4;

        struct udphdr *udp = (struct udphdr*)(buf + 
            sizeof(struct ethhdr) + ip_header_len);

        if (ntohs(udp->dest) != 12345)
            continue;

        if (ntohs(udp->source) != 8080)
            continue;

        int udp_len = ntohs(udp->len);
        int payload_len = udp_len - sizeof(struct udphdr);

        char *payload = buf + sizeof(struct ethhdr) + 
            ip_header_len + sizeof(struct udphdr);

        printf("Server: %.*s\n", payload_len, payload);

        break;
    }

    close(sock);
}