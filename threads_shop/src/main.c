#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include "shop.h"

kiosk_t kiosks[KIOSKS];
int buyers_done = 0;
pthread_mutex_t done_mutex = PTHREAD_MUTEX_INITIALIZER;

int main() 
{
    srand(time(NULL));

    // инициализация ларьков
    for (int i = 0; i < KIOSKS; i++) 
    {
        kiosks[i].goods = 900 + rand() % 201;
        pthread_mutex_init(&kiosks[i].mutex, NULL);
    }

    pthread_t buyers[BUYERS];
    pthread_t worker;

    // создание покупателей
    for (int i = 0; i < BUYERS; i++) 
    {
        int* id = malloc(sizeof(int));
        *id = i;
        pthread_create(&buyers[i], NULL, buyer_thread, id);
    }

    // создание погрузчика
    pthread_create(&worker, NULL, worker_thread, NULL);

    // Ждём покупателей
    for (int i = 0; i < BUYERS; i++) 
    {
        pthread_join(buyers[i], NULL);
    }

    // После завершения покупателей
    pthread_cancel(worker);
    pthread_join(worker, NULL);

    printf("All buyers' needs were met. Completion...\n");
    return 0;
}