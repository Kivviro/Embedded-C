/**
 * @file server.c
 * @brief UDP raw socket echo-server with client tracking.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

#define SERVER_PORT 8080

/** @brief Flag for main loop control (set to 0 on SIGINT). */
volatile sig_atomic_t running = 1;

/**
 * @struct ClientData
 * @brief Stores information about a connected client.
 */
typedef struct
{
    uint32_t ip; /**< Client IPv4 address (network byte order). */
    uint16_t port; /**< Client UDP port (network byte order). */
    int counter; /**< Number of received packets from this client. */
} ClientData;

/**
 * @brief SIGINT handler to stop server loop.
 * @param sig Signal number.
 */
void sigint_handler(int sig)
{
    running = 0;
}

/**
 * @brief Initializes dynamic client list.
 * @param list Pointer to client array.
 * @param capacity Initial capacity of array.
 */
void init_clients(ClientData **list, size_t *capacity)
{
    *capacity = 16;
    *list = calloc(*capacity, sizeof(**list));

    if (*list == NULL)
    {
        perror("null_clients");
        exit(1);
    }
}

/**
 * @brief Expands client array when capacity is reached.
 * @param list Pointer to client array.
 * @param capacity Pointer to current capacity.
 */
void expand_clients(ClientData **list, size_t *capacity)
{
    size_t old_capacity = *capacity;
    *capacity *= 2;

    ClientData *tmp = realloc(*list, (*capacity) * sizeof(**list));

    if (tmp == NULL)
    {
        free(*list);
        exit(1);
    }

    *list = tmp;

    memset((*list) + old_capacity, 0, (*capacity - old_capacity) * sizeof(**list));
}

/**
 * @brief Finds existing client or adds a new one.
 *
 * Also detects disconnect message "__CLIENT_DISCONNECT__".
 *
 * @param list Client list.
 * @param count Current number of clients.
 * @param capacity Current capacity of list.
 * @param ip Source IP.
 * @param port Source port.
 * @param payload_len UDP payload length.
 * @param payload UDP payload data.
 * @param disconnected Flag set if client requested disconnect.
 *
 * @return Index of client in array or -1 on failure.
 */
int find_or_add_client(ClientData **list, size_t *count, size_t *capacity,
    uint32_t ip, uint16_t port, int payload_len, char *payload, int *disconnected)
{
    for (int i = 0; i < *count + 1; i++)
    {
        if (i == *count)
        {
            if (*count == *capacity)
                expand_clients(list, capacity);

            (*list)[i].ip = ip;
            (*list)[i].port = port;
            (*list)[i].counter = 1;

            (*count)++;

            printf("[SERVER] Client added\n");
            return i;
        }

        if ((*list)[i].ip == ip && (*list)[i].port == port)
        {
            if (payload_len == strlen("__CLIENT_DISCONNECT__") &&
                memcmp(payload, "__CLIENT_DISCONNECT__", payload_len) == 0)
            {
                (*list)[i].counter = 0;
                *disconnected = 1;
                return i;
            }

            (*list)[i].counter++;
            return i;
        }
    }

    return -1;
}

/**
 * @brief Parses raw UDP packet from IP buffer.
 *
 * @param buffer Raw packet buffer.
 * @param ip Parsed IP header.
 * @param udp Parsed UDP header.
 * @param ip_header_len Size of IP header.
 * @param payload UDP payload pointer.
 * @param payload_len UDP payload length.
 *
 * @return 1 if valid UDP packet for server, otherwise 0.
 */
int parse_udp(char *buffer, struct iphdr **ip, struct udphdr **udp, 
    int *ip_header_len, char **payload, int *payload_len)
{
    *ip = (struct iphdr *)buffer;

    if ((*ip)->protocol != IPPROTO_UDP)
        return 0;

    *ip_header_len = (*ip)->ihl * 4;

    *udp = (struct udphdr *)(buffer + *ip_header_len);

    if (ntohs((*udp)->dest) != SERVER_PORT)
        return 0;

    int udp_len = ntohs((*udp)->len);

    *payload = buffer + *ip_header_len + sizeof(struct udphdr);
    *payload_len = udp_len - sizeof(struct udphdr);

    return 1;
}

/**
 * @brief Builds UDP response packet.
 *
 * @param out Response payload string.
 * @param src_port Destination port.
 * @param src_ip Destination IP.
 * @param packet Output packet buffer.
 * @param packet_size_limit Size of output buffer.
 *
 * @return Total packet size.
 */
int build_packet(char *out, uint16_t src_port, uint32_t src_ip, char *packet, 
    int packet_size_limit)
{
    struct udphdr *udp_server = (struct udphdr *)packet;
    char *payload_server = packet + sizeof(struct udphdr);

    size_t max_payload = packet_size_limit - sizeof(struct udphdr);
    size_t len = strlen(out);

    if (len > max_payload)
        len = max_payload;

    memcpy(payload_server, out, len);

    udp_server->source = htons(SERVER_PORT);
    udp_server->dest = htons(src_port);
    udp_server->len = htons(sizeof(struct udphdr) + len);
    udp_server->check = 0;

    return sizeof(struct udphdr) + len;
}

/**
 * @brief Entry point of the UDP raw socket server.
 *
 * Initializes signal handling for graceful shutdown, sets up a raw UDP socket,
 * prepares client storage, and enters the main packet-processing loop.
 *
 * @return Always returns 0 on normal shutdown.
 */
int main()
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    ClientData *clients_list = NULL;
    size_t count_of_clients = 0;
    size_t capacity = 0;

    init_clients(&clients_list, &capacity);

    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(sock, (struct sockaddr *)&addr, sizeof(addr));

    char buffer[1024];

    printf("[SYSTEM] Server started.\n");

    while (running)
    {
        int disconeted = 0;

        struct sockaddr_in client;
        socklen_t len = sizeof(client);

        int n = recvfrom(sock, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&client, &len);

        if (n < 0)
        {
            if (!running)
                break;

            continue;
        }

        struct iphdr *ip;
        struct udphdr *udp;
        int ip_header_len;
        char *payload;
        int payload_len;

        if (!parse_udp(buffer, &ip, &udp, &ip_header_len, &payload, &payload_len))
            continue;

        uint32_t src_ip = ip->saddr;
        uint16_t src_port = ntohs(udp->source);

        char out[1500];

        int idx = find_or_add_client(&clients_list, &count_of_clients, &capacity,
        src_ip, src_port, payload_len, payload, &disconeted);

        if (disconeted)
        {
            printf("[SERVER] Client removed\n");
            continue;
        }

        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &src_ip, ip_str, sizeof(ip_str));

        printf("[SERVER] Client data: %s:%d | %d\n", 
            ip_str, src_port, clients_list[idx].counter);

        snprintf(out, sizeof(out), "%.*s %d", payload_len, payload, 
            clients_list[idx].counter);

        printf("[SERVER] Modifire message: '%s'\n", out);

        struct sockaddr_in dst = {0};
        dst.sin_family = AF_INET;
        dst.sin_addr.s_addr = src_ip;

        char packet[1024];

        int packet_size = build_packet(out, src_port, src_ip, 
            packet, sizeof(packet));

        sendto(sock, packet, packet_size, 0, (struct sockaddr *)&dst, sizeof(dst));

        printf("[SERVER] Send to client...\n");
    }

    free(clients_list);
    close(sock);
}