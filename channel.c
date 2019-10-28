#include <pthread.h>
#include "channel.h"

// Starting length of the queue.
#define QUEUE_CAPACITY 50

/**
 * Creates (and returns) a new, empty queue, with no data in it.
 * @return new empty queue struct
 */
struct Queue new_queue(void) {
    struct Queue output;

    output.data = malloc(sizeof(void*) * QUEUE_CAPACITY);
    output.readEnd = -1; // queue is empty
    output.writeEnd = 0; // put first piece of data at the start of the queue

    return output;
}

/**
 * Cleans up all data in a queue.
 * @param queue: a pointer to the queue being destroyed
 * @param clean: pointer to a function for cleanup of queue elements,
 *      if NULL, no cleanup occurs
 */
void destroy_queue(struct Queue* queue, void (*clean)(void*)) {
    void* data;
    while (read_queue(queue, &data)) {
        clean(data);
    }

    free(queue->data);
}

/**
 * Attempts to write a piece of data to the queue.
 * @param queue: a pointer to the queue to write to
 * @param data: the data to be written
 * @return: true if the attempt was successful, false if the queue was full
 *      and unable to be written to
 */
bool write_queue(struct Queue* queue, void* data) {
    if (queue->writeEnd == queue->readEnd) {
        // queue is full
        return false;
    }

    queue->data[queue->writeEnd] = data;

    // if queue was empty, signal that the queue is no longer empty
    if (queue->readEnd == -1) {
        queue->readEnd = queue->writeEnd;
    }

    queue->writeEnd = (queue->writeEnd + 1) % QUEUE_CAPACITY;

    return true;
}

/**
 * Attempts to read a piece of data from the queue.
 * @param queue: pointer to the queue to write to
 * @param output: pointer to where the data should be stored on a
 *      successful queue.
 * @return true if read is successful, and sets *output to the read
 * data. False if queue is empty (no action taken)
 */
bool read_queue(struct Queue* queue, void** output) {
    if (queue->readEnd == -1) {
        // queue is empty
        return false;
    }

    *output = queue->data[queue->readEnd];

    queue->readEnd = (queue->readEnd + 1) % QUEUE_CAPACITY;

    if (queue->readEnd == queue->writeEnd) {
        queue->readEnd = -1;
    }

    return true;
}

/**
 * Creates a new empty channel, with no data in it.
 * @return: pointer to the newly created channel struct
 */
struct Channel* new_channel(void) {
    struct Channel* output = malloc(sizeof(struct Channel));

    sem_t signal;
    pthread_mutex_t queueLock;

    sem_init(&signal, 0, 1);
    pthread_mutex_init(&queueLock, NULL);

    output->queue = new_queue();
    output->signal = signal;
    output->queueLock = queueLock;

    return output;
}

/**
 * Destroys an old channel, cleaning up its data.
 * @param channel: a pointer to the channel struct to destroy
 * @param clean: a pointer to a function to use for clean up of elements
 *      in the channel (e.g. free), if NULL no cleanup occurs.
 */
void destroy_channel(struct Channel* channel, void (*clean)(void*)) {

    destroy_queue(&channel->queue, clean);
    pthread_mutex_destroy(&channel->queueLock);
    sem_destroy(&channel->signal);
}

/**
 * Attempts to write a piece of data to the channel.
 * @param channel: a pointer to the channel to write to
 * @param data: the data to write to the channel
 * @return true if the writing attempt was successful, false
 *      if the channel is full, and writing is unsuccessful.
 */
bool write_channel(struct Channel* channel, void* data) {

    pthread_mutex_lock(&channel->queueLock);
    bool output = write_queue(&channel->queue, data);
    pthread_mutex_unlock(&channel->queueLock);

    sem_post(&channel->signal);
    //fprintf(stderr, "STDERR: Writing to channel...\n");

    return output;
}

/**
 * Attempts to read a piece of data from the channel.
 * @param: a pointer to the channel to read from
 * @param: a pointer to where read data from the channel should be stored
 * @return true if read successful, sets *output to the read data. False,
 *      if channel is empty, and no read occurs.
 */
bool read_channel(struct Channel* channel, void** out) {

    sem_wait(&channel->signal);
    //fprintf(stderr, "STDERR: Past semaphore...\n");

    pthread_mutex_lock(&channel->queueLock);
    bool output = read_queue(&channel->queue, out);
    pthread_mutex_unlock(&channel->queueLock);
    //fprintf(stderr, "STDERR: Reading from channel...\n");

    return output;
}
