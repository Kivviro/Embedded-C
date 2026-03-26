#ifndef SHOP_H
#define SHOP_H

#include <pthread.h>

#define KIOSKS 5
#define BUYERS 3

typedef struct 
{
    int goods;
    pthread_mutex_t mutex;
} kiosk_t;

extern kiosk_t kiosks[KIOSKS];

extern int buyers_done;
extern pthread_mutex_t done_mutex;

void* buyer_thread(void* arg);
void* worker_thread(void* arg);

#endif