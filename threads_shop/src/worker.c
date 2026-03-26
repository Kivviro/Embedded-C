#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "shop.h"

void* worker_thread(void* arg) 
{
    (void)arg;

    printf("The worker gets to work.\n");

    while (1) 
    {
        pthread_testcancel(); // важно!

        int index = rand() % KIOSKS;

        if (pthread_mutex_trylock(&kiosks[index].mutex) == 0) 
        {
            kiosks[index].goods += 200;

            printf("A worker went into the K%d kiosk and unloaded the goods. There are now %d goods there.\n",
                   index, kiosks[index].goods);

            pthread_mutex_unlock(&kiosks[index].mutex);
        }

        printf("The worker stepped out for a 1 second smoke break.\n");
        sleep(1);

        pthread_mutex_lock(&done_mutex);
        if (buyers_done == BUYERS) 
        {
            pthread_mutex_unlock(&done_mutex);
            break;
        }
        pthread_mutex_unlock(&done_mutex);
    }

    return NULL;
}