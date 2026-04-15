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

    const char *message = "Hello!";

    key_t key = ftok(ftok_path, id);

    int shmid = shmget(key, SIZE, 0666);
    char *buf = shmat(shmid, NULL, 0);

    int semid = semget(key, 2, 0666);

    struct sembuf wait_srv = {0, -1, 0};
    semop(semid, &wait_srv, 1);

    printf("Client read: \"%s\"\n", buf);

    snprintf(buf, SIZE, message);

    struct sembuf post_cli = {1, 1, 0};
    semop(semid, &post_cli, 1);

    shmdt(buf);
    
    return 0;
}

