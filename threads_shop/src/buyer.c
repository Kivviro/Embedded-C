#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "shop.h"

void* buyer_thread(void* arg) 
{
    int id = *(int*)arg;
    free(arg);

    int need = 9900 + rand() % 201;

    printf("Buyer %d has appeared on the horizon; their need is %d\n", id, need);

    while (need > 0) 
    {
        int found = 0;

        for (int i = 0; i < KIOSKS; i++) 
        {
            if (pthread_mutex_trylock(&kiosks[i].mutex) == 0) 
            {
                found = 1;

                printf("Buyer %d entered kiosk K%d; the kiosk contains %d goods\n",
                       id, i, kiosks[i].goods);

                while (kiosks[i].goods > 0 && need > 0) 
                {
                    kiosks[i].goods--;
                    need--;
                }

                printf("Buys %d has left kiosk K%d; their needs is now =%d\n",
                       id, i, need);

                pthread_mutex_unlock(&kiosks[i].mutex);
                break;
            }
        }

        if (!found) 
        {
            sleep(2);
        } 
        else if (need > 0) 
        {
            printf("Buyer %d passed out in the side street for 2 seconds\n", id);
            sleep(2);
        }
    }

    printf("Customer %d is satisfied\n", id);

    pthread_mutex_lock(&done_mutex);
    buyers_done++;
    pthread_mutex_unlock(&done_mutex);

    return NULL;
}