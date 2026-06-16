#define _POSIX_C_SOURCE 200809L
#include "common.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

int main()
{
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(shm_chat_t));

    shm_chat_t *shm = mmap(NULL, sizeof(shm_chat_t), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    memset(shm, 0, sizeof(*shm));

    pthread_mutexattr_t mattr;
    pthread_condattr_t cattr;

    pthread_mutexattr_init(&mattr);
    pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);

    pthread_condattr_init(&cattr);
    pthread_condattr_setpshared(&cattr, PTHREAD_PROCESS_SHARED);

    pthread_mutex_init(&shm->lock, &mattr);
    pthread_cond_init(&shm->cond, &cattr);

    printf("Server started (SHM)\n");

    while (1)
        pause();

    return 0;
}