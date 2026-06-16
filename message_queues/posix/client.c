// client
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <mqueue.h>
#include <string.h>

int main() 
{
    mqd_t mq_s = mq_open("/server_queue", O_RDWR);
    mqd_t mq_с = mq_open("/client_queue", O_RDWR);

    char buf[100];
    mq_receive(mq_s, buf, 100, NULL);
    printf("Server says: %s\n", buf);

    mq_send(mq_с, "Hello!", strlen("Hello!") + 1, 0);

    mq_close(mq_s);
    mq_close(mq_с);

    return 0;
}