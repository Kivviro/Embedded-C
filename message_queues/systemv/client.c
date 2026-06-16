// client
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>

struct msgbuf 
{
    long mtype;
    char mtext[100];
};

int main() 
{
    key_t key = ftok("queue", 65);
    int msqid = msgget(key, 0666);

    struct msgbuf msg;

    msgrcv(msqid, &msg, sizeof(msg.mtext), 1, 0);
    printf("Server says: %s\n", msg.mtext);

    msg.mtype = 2;
    strcpy(msg.mtext, "Hello!");
    msgsnd(msqid, &msg, sizeof(msg.mtext), 0);

    return 0;
}