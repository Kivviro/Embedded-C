#include <stdio.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

int main(void)
{
    const int SIZE = 4096;
    const int id = 1;
    const char *ftok_path = "/tmp";

    const char *message = "Hi!";

    key_t key = ftok(ftok_path, id);

    int shmid = shmget(key, SIZE, IPC_CREAT | 0666);
    char *buf = shmat(shmid, NULL, 0);

    int semid = semget(key, 2, IPC_CREAT | 0666);
    unsigned short init[2] = {0,0};
    semctl(semid, 0, SETALL, init);

    snprintf(buf, SIZE, message);

    struct sembuf post_srv = {0, 1, 0};
    semop(semid, &post_srv, 1);

    struct sembuf wait_cli = {1, -1, 0};
    semop(semid, &wait_cli, 1);

    printf("Server read: \"%s\"\n", buf);

    shmdt(buf);
    shmctl(shmid, IPC_RMID, NULL);
    semctl(semid, 0, IPC_RMID);
    return 0;
}
