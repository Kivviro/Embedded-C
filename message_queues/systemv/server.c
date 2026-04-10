// server
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
    int msqid = msgget(key, 0666 | IPC_CREAT);

    struct msgbuf msg;

    msg.mtype = 1;
    strcpy(msg.mtext, "Hi!");
    msgsnd(msqid, &msg, sizeof(msg.mtext), 0);

    msgrcv(msqid, &msg, sizeof(msg.mtext), 2, 0);
    printf("Client says: %s\n", msg.mtext);

    msgctl(msqid, IPC_RMID, NULL);

    return 0;
}