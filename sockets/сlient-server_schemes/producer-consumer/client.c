#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define LISTENER_PORT 8080

int create_listener(int *out_port) 
{
    int s = socket(AF_INET, SOCK_STREAM, 0);

    if (s < 0) 
        return -1;

    int opt=1; 
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in a; memset(&a,0,sizeof(a));

    a.sin_family = AF_INET;
    a.sin_addr.s_addr = inet_addr("127.0.0.1");
    a.sin_port = 0;

    if (bind(s, (struct sockaddr*)&a, sizeof(a)) < 0) 
    { 
        close(s); 
        return -1; 
    }
    listen(s, 1);
    socklen_t len = sizeof(a);
    getsockname(s, (struct sockaddr*)&a, &len);
    *out_port = ntohs(a.sin_port);

    return s;
}

int connect_to_listener_and_send_port(int port) 
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in la; memset(&la,0,sizeof(la));
    la.sin_family = AF_INET;
    la.sin_port = htons(LISTENER_PORT);
    inet_pton(AF_INET, "127.0.0.1", &la.sin_addr);

    if (connect(sock, (struct sockaddr*)&la, sizeof(la)) < 0) 
    { 
        close(sock); 
        return -1; 
    }

    int netport = htonl(port);
    write(sock, &netport, sizeof(netport));

    char buf[16]; 
    read(sock, buf, sizeof(buf));
    close(sock);
    return 0;
}

int main() 
{
    int client_listen_port;
    int lfd = create_listener(&client_listen_port);
    if (lfd < 0) 
        return 1; 
    
    printf("Client listening on 127.0.0.1:%d\n", client_listen_port);

    if (connect_to_listener_and_send_port(client_listen_port) < 0) 
    {
        close(lfd);
        return 1;
    }
    printf("Request sent to listener, waiting for service to connect...\n");

    int sfd = accept(lfd, NULL, NULL);
    if (sfd < 0) 
    { 
        close(lfd); 
        return 1; 
    }

    const char *req = "time\n";
    write(sfd, req, strlen(req));

    char buf[1024];
    ssize_t n = read(sfd, buf, sizeof(buf)-1);

    if (n > 0)
    {
        buf[n] = 0;
        printf("Server time: %s", buf);
    }
    
    close(sfd);
    close(lfd);
    return 0;
}