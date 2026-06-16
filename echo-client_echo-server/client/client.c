 /**
 * @file client.c
 * @brief Raw UDP echo-client with receiver thread and interactive input.
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/udp.h>

#define CLIENT_PORT 12345
#define SERVER_PORT 8080
#define MAX_PACKET 1024

/** @brief Global flag for program execution state. */
volatile sig_atomic_t running = 1;

/** @brief Raw socket file descriptor. */
int sockfd;

/** @brief Server address structure. */
struct sockaddr_in server_addr;

/**
 * @brief Sends raw UDP packet with payload.
 *
 * Builds UDP header manually and sends packet via raw socket.
 *
 * @param message Null-terminated message string.
 */
void send_udp_message(const char *message)
{
    char packet[MAX_PACKET];

    memset(packet, 0, sizeof(packet));

    struct udphdr *udp = (struct udphdr *)packet;

    char *payload = packet + sizeof(struct udphdr);

    strcpy(payload, message);

    udp->source = htons(CLIENT_PORT);
    udp->dest = htons(SERVER_PORT);
    udp->len = htons(sizeof(struct udphdr) + strlen(message));
    udp->check = 0;

    int packet_size = sizeof(struct udphdr) + strlen(message);

    if (sendto(sockfd, packet, packet_size, 0, (struct sockaddr *)&server_addr,
    sizeof(server_addr)) < 0)
    {
        perror("sendto");
    }
}

/**
 * @brief SIGINT handler.
 *
 * Stops main loop and receiver thread.
 *
 * @param sig Signal number.
 */
void sigint_handler(int sig)
{
    (void)sig;
    running = 0;
}

/**
 * @brief Thread function for receiving UDP packets.
 *
 * Continuously listens on raw socket and extracts UDP payload
 * addressed to CLIENT_PORT from SERVER_PORT.
 *
 * @param arg Unused argument.
 * @return NULL.
 */
void *receiver_thread(void *arg)
{
    (void)arg;

    char buf[65535];

    while (running)
    {
        int n = recvfrom(sockfd, buf, sizeof(buf), 0, NULL, NULL);

        if (n <= 0)
            continue;

        unsigned char ver_ihl = buf[0];

        int ip_header_len = (ver_ihl & 0x0F) * 4;

        struct udphdr *udp = (struct udphdr *)(buf + ip_header_len);

        if (ntohs(udp->dest) != CLIENT_PORT)
            continue;

        if (ntohs(udp->source) != SERVER_PORT)
            continue;

        int udp_len = ntohs(udp->len);

        int payload_len = udp_len - sizeof(struct udphdr);

        if (payload_len <= 0)
            continue;

        char *payload = buf + ip_header_len + sizeof(struct udphdr);

        printf("\n[SERVER] %.*s\n", payload_len, payload);

        printf(">");

        fflush(stdout);
    }

    return NULL;
}

/**
 * @brief Program entry point.
 *
 * Initializes raw socket, starts receiver thread and handles user input loop.
 *
 * @return 0 on success, non-zero on error.
 */
int main()
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);

    if (sockfd < 0)
    {
        perror("socket");
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);


    pthread_t recv_thread;

    if (pthread_create(&recv_thread, NULL, receiver_thread, NULL) != 0)
    {
        perror("pthread_create");
        close(sockfd);
        return 1;
    }

    printf("[SYSTEM] Client started.\n");
    printf("[SYSTEM] Type messages and press Enter.\n");
    printf("[SYSTEM] Press Ctrl+C to exit.\n\n");

    char input[512];
    printf(">");
    fflush(stdout);

    // input cycle
    while (running)
    {
        if (fgets(input, sizeof(input), stdin) == NULL)
        {
            if(!running)
                break;
            
            continue;
        }   
        
        input[strcspn(input, "\n")] = '\0';

        if (strlen(input) == 0)
            continue;

        send_udp_message(input);
    }

    // terminated dsct msg for server
    if (!running)
    {
        printf("[CLIENT] Sending disconnect notification...\n");

        send_udp_message("__CLIENT_DISCONNECT__");
    }

    // thread waiting
    running = 0;

    pthread_cancel(recv_thread);
    pthread_join(recv_thread, NULL);

    close(sockfd);

    printf("[SYSTEM] Client terminated.\n");

    return 0;
}