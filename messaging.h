#ifndef MESSAGING_H
#define MESSAGING_H

#include <stdio.h>
#include <stdbool.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

struct LinkedList;

bool check_im_message(char* message);

bool check_connect_message(char* message);

bool check_deliver_withdraw_message(char* message, char* commandString);

void handle_deliver_withdraw_message(char* message,
        struct LinkedList* firstResource, pthread_mutex_t* dataLock,
        int command);

void handle_transfer_message(char* message,
        struct LinkedList* firstResource, struct LinkedList* thisDepot,
        pthread_mutex_t* dataLock);

bool check_transfer_message(char* message);

bool check_defer_message(char* message);

bool check_execute_message(char* message);

void handle_execute_message(char* message, struct LinkedList* firstDeferral,
        pthread_mutex_t* dataLock);

#endif //MESSAGING_H
