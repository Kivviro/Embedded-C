#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <semaphore.h>


int main()
{
    const int SIZE = 4096;
    const char *shm_name  = "smo";

    const char *sem_srv_name = "/sem_server";
    const char *sem_cli_name = "/sem_client";
    const char *message = "Hi!";

    int shm_fd;
    char *ptr;
    sem_t *sem_srv;
    sem_t *sem_cli;

    sem_srv = sem_open(sem_srv_name, O_CREAT | O_EXCL, 0666, 0);
    sem_cli = sem_open(sem_cli_name, O_CREAT | O_EXCL, 0666, 0);

    shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, SIZE);

    ptr = mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    snprintf(ptr, SIZE, "%s", message);

    sem_post(sem_srv);

    sem_wait(sem_cli);

    printf("Server read: \"%s\"\n", ptr);

    munmap(ptr, SIZE);

    close(shm_fd);
    shm_unlink(shm_name);

    sem_close(sem_srv);
    sem_unlink(sem_srv_name);
    sem_close(sem_cli);
    sem_unlink(sem_cli_name);

    return 0;
}