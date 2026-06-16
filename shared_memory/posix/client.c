#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <semaphore.h>


int main()
{
    const int SIZE = 4096;
    const char *shm_name = "smo";
    const char *sem_srv_name = "/sem_server";
    const char *sem_cli_name = "/sem_client";
    const char *reply = "Hello!";

    int shm_fd;
    char *ptr;
    sem_t *sem_srv;
    sem_t *sem_cli;

    sem_srv = sem_open(sem_srv_name, 0);
    sem_cli = sem_open(sem_cli_name, 0);

    shm_fd = shm_open(shm_name, O_RDWR, 0);
    ptr = mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    sem_wait(sem_srv);

    printf("Client read: \"%s\"\n", ptr);

    snprintf(ptr, SIZE, "%s", reply);

    sem_post(sem_cli);

    munmap(ptr, SIZE);
    close(shm_fd);
    sem_close(sem_srv);
    sem_close(sem_cli);
    
    return 0;
}