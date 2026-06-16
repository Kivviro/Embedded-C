#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int main()
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    int flag = 1;
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &flag, sizeof(flag));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = inet_addr("255.255.255.255");

    for(int counter = 0; counter < 10; counter++) 
    {
        char msg[256];

        snprintf(msg, sizeof(msg), "This is sring №%d", counter);

        sendto(sock, msg, strlen(msg), 0,(struct sockaddr*)&addr, sizeof(addr));
        printf("Send: %s\n", msg);

        sleep(1);
    }

    close(sock);
    return 0;
}