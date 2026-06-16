// server
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <mqueue.h>
#include <string.h>

int main() 
{
    struct mq_attr attr;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = 100;

    mqd_t mq_s = mq_open("/server_queue", O_CREAT | O_RDWR, 0666, &attr);
    mqd_t mq_c = mq_open("/client_queue", O_CREAT | O_RDWR, 0666, &attr);

    mq_send(mq_s, "Hi!", strlen("Hi!") + 1, 0);

    char buf[100];
    mq_receive(mq_c, buf, 100, NULL);
    printf("Client says: %s\n", buf);

    mq_close(mq_s);
    mq_close(mq_c);

    mq_unlink("/server_queue");
    mq_unlink("/client_queue");

    return 0;
}