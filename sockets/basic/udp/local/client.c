#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>


#define SERVER_PATH "/tmp/udp_server_sock"
#define CLIENT_PATH "/tmp/udp_client_sock"

int main()
{
    int sock = socket(AF_LOCAL, SOCK_DGRAM, 0);

    struct sockaddr_un client = {0};
    client.sun_family = AF_LOCAL;
    strcpy(client.sun_path, CLIENT_PATH);

    unlink(CLIENT_PATH);
    bind(sock, (struct sockaddr*)&client, sizeof(client));

    struct sockaddr_un server = {0};
    server.sun_family = AF_LOCAL;
    strcpy(server.sun_path, SERVER_PATH);

    char buf[1024];

    sendto(sock, "Hello!\n", 7, 0, (struct sockaddr*)&server, sizeof(server));

    int n = recvfrom(sock, buf, sizeof(buf), 0, NULL, NULL);

    buf[n] = 0;
    printf("Server: %s", buf);

    unlink(CLIENT_PATH);
    close(sock);
}