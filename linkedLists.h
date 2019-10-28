#ifndef LINKED_LISTS_H
#define LINKED_LISTS_H

#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <string.h>
#include <stdbool.h>

/**
 * Struct which describes a single deferred operation to be handled later.
 * Key is negative 1 if this deferral has been used, and executed is set to
 * true upon  an execute command.
 */
struct Deferral {
    char* operation;
    int key;
    bool executed;
};

/**
 * Struct which describes a single resource for this depot. Any amount
 * of resources could exist for a single depot.
 */
struct Resource {
    int quantity;
};

/**
 * Struct which describes an existing connection between this depot and
 * another, including information about the other depot, and the streams
 * to contact that depot with.
 */
struct Depot {
    char* port;
    FILE* to;
    FILE* from;
    pthread_t readerId;
    pthread_t writerId;
};

/**
 * Union which allows both a resource and depot type struct to be identified
 * as a LinkedList struct. These types are mutually exclusive.
 */
union Type {
    struct Resource resource;
    struct Depot depot;
    struct Deferral deferral;
};

/**
 * Struct representing a linked list data structure. This list can contain
 * either depot structs or resource structs as defined by the type union.
 * New elements are linked by adding a pointer to a new struct for the last
 * element in the linked list. To search through the list, you must start
 * from the first element.
 */
struct LinkedList {
    char* name;
    union Type type;
    struct LinkedList* next;

};

struct LinkedList* search_deferrals_by_key(int key,
        struct LinkedList* firstDeferral);

int count_items_in_list(struct LinkedList* first);

struct LinkedList* add_item(struct LinkedList* first);

struct LinkedList* search_list_by_name(char* search,
        struct LinkedList* firstItem);

void free_linked_list(struct LinkedList* first);

#endif // LINKED_LISTS_H
