#ifndef CHANNEL_H
#define CHANNEL_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>

/**
 * A first in, first out (FIFO) queue, holding messages between depots to be
 * handled by a read/action thread for each connection. This data structure
 * (by itself) is not threadsafe.
 */
struct Queue {
    // The data contained in the queue - as an array.
    void** data;
    // An offset within that array that points to the beginning of the queue
    // (where new data should be written).
    int writeEnd;
    // An offset within that array that points to the end of the queue (where
    // old data should be read).
    int readEnd;
};

struct Queue new_queue(void);

void destroy_queue(struct Queue* queue, void (*clean)(void*));

bool write_queue(struct Queue* queue, void* data);

bool read_queue(struct Queue* queue, void** output);


/**
 * A threadsafe channel between two depots. Data can be written to the
 * channel or read from the channel at different times, by read/write threads
 * safely (using a semaphore and mutex).
 */
struct Channel {
    sem_t signal;
    pthread_mutex_t queueLock;
    struct Queue queue;
};

struct Channel* new_channel(void);

void destroy_channel(struct Channel* channel, void (*clean)(void*));

bool write_channel(struct Channel* channel, void* data);

bool read_channel(struct Channel* channel, void** output);

#endif //CHANNEL_H
