#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCK_PATH "/tmp/lockal_tcp_sock"

int main() 
{
    int sock = socket(AF_LOCAL, SOCK_STREAM, 0);

    struct sockaddr_un addr = {0};
    addr.sun_family = AF_LOCAL;
    strcpy(addr.sun_path, SOCK_PATH);

    connect(sock, (struct sockaddr*)&addr, sizeof(addr));

    char buf[1024];

    send(sock, "Hello!\n", 7, 0);

    int n = recv(sock, buf, sizeof(buf)-1, 0);
    
    buf[n] = 0;
    printf("Server: %s", buf);

    close(sock);
}