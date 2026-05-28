#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>

int main()
{
    srand(time(NULL));

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(sock, (struct sockaddr*)&addr, sizeof(addr));

    
    char buf[1024];
    struct sockaddr_in client;
    socklen_t len = sizeof(client);

    while(1)
    {
        int n = recvfrom(sock, buf, sizeof(buf) - 1, 0, 
        (struct sockaddr*)&client, &len);

        if (n <= 0)
            continue;

        buf[n] = 0;

        printf("Original: %s\n", buf);

        if (n > 1)
        {
            int pos = rand() % (n - 1);
            buf[pos] = '*';
        }

        printf("Modified: %s\n", buf);

        sendto(sock, buf, strlen(buf), 0, (struct sockaddr*)&client, len);
    }

    close(sock);
}