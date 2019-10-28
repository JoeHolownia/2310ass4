#ifndef NETWORK_H
#define NETWORK_H

#include <stdio.h>
#include <stdbool.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

struct LinkedList;
struct Channel;

/**
 * Connection wrapper struct, which contains all integral information
 * for a single connection to be set up between two depots. This is passed
 * to threads and subsequent functions that deal with depot communications.
 */
struct ConnectionWrapper {

    struct LinkedList* thisDepot;
    struct LinkedList* connectedDepot;
    struct LinkedList* firstResource;
    struct LinkedList* firstDeferral;
    struct Channel* channel;
    pthread_mutex_t* dataLock;
    int serverSocket;
    FILE* to;
    FILE* from;
};

void* defer_thread(void* arg);

void handle_defer_message(char* message,
        struct ConnectionWrapper* connection);

bool handle_im_message(char* message,
        struct ConnectionWrapper* connection);

void handle_connect_message(char* message,
        struct ConnectionWrapper* connection);

void handle_messages(char* message, struct ConnectionWrapper* connection);

void* reader_thread(void* arg);

void* action_thread(void* arg);

pthread_t start_server(struct LinkedList* thisDepot,
        struct LinkedList* firstResource, struct LinkedList* firstDeferral,
        pthread_mutex_t* dataLock);

int connect_to_depot(const char* port, struct ConnectionWrapper* connection);

#endif //NETWORK_H
