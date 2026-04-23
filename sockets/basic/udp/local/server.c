#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>

#define SERVER_PATH "/tmp/udp_server_sock"

void handler(int sig)
{
    unlink(SERVER_PATH);
    _exit(0);
}

int main()
{
    signal(SIGINT, handler);
    int sock = socket(AF_LOCAL, SOCK_DGRAM, 0);

    struct sockaddr_un addr = {0};
    addr.sun_family = AF_LOCAL;
    strcpy(addr.sun_path, SERVER_PATH);

    unlink(SERVER_PATH);
    bind(sock, (struct sockaddr*)&addr, sizeof(addr));

    char buf[1024];
    struct sockaddr_un client;
    socklen_t len = sizeof(client);

    while(1)
    {
        int n = recvfrom(sock, buf, sizeof(buf), 0, 
        (struct sockaddr*)&client, &len);

        buf[n] = 0;
        printf("Client: %s", buf);

        sendto(sock, "Hi!\n", 4, 0, (struct sockaddr*)&client, len);
    }
    
    close(sock);
}