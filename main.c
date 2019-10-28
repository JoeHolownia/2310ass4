#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <semaphore.h>
#include <signal.h>

#include "linkedLists.h"
#include "network.h"
#include "util.h"

#define MIN_ARGS 2
#define NUM_ARG_ERR 1
#define NAME_ERR 2
#define QUANTITY_ERR 3

// Global Boolean Variable to detect SIGHUP
bool sigHupDetected = false;

// Global Boolean Variable to detect SIGPIPE
bool sigPipeDetected = false;

/**
 * Signal handler for SIGHUP signal, which sets global flag
 * for the displaying of depot data.
 *
 * @param signalNum: parameter in case of multiple signals
 */
void handle_sighup(int signalNum) {
    sigHupDetected = true;
}

/**
 * Signal handler for SIGPIPE signal. Dummy handler to stop
 * program crash from SIGPIPE.
 *
 * @param signalNum: parameter in case of multiple signals
 */
void handle_sigpipe(int signalNum) {
    sigPipeDetected = true;
}

/**
 * Displays this depot's current stock of (non-zero) goods in
 * lexicographic order, and the connected neighbours of this
 * depot in lexicographic order, to stdout.
 *
 * @param thisDepot: start of linked list of all connected depots
 * @param firstResource: start of linked list of all resources
 */
void display_depot_data(struct LinkedList* thisDepot,
        struct LinkedList* firstResource) {

    struct LinkedList* node;
    char** goodNames;
    int quantity;
    char** neighbours;
    int goodCount, neighbourCount = 0;

    printf("Goods:\n"); // create goods list for sorting
    goodCount = count_items_in_list(firstResource);
    goodNames = malloc(sizeof(char*) * goodCount);

    node = firstResource;
    for (int i = 0; i < goodCount; i++) {
        goodNames[i] = node->name;
        node = node->next;
    }

    qsort(goodNames, goodCount, sizeof(char*), string_compare);
    for (int i = 0; i < goodCount; i++) {
        node = search_list_by_name(goodNames[i], firstResource);
        quantity = node->type.resource.quantity;
        if (quantity != 0) {
            printf("%s %i\n", goodNames[i], quantity);
        }
    }

    printf("Neighbours:\n"); // create neighbours list for sorting
    neighbourCount = count_items_in_list(thisDepot) - 1;
    neighbours = malloc(sizeof(char*) * neighbourCount);
    if (neighbourCount < 1) {
        fflush(stdout); // exit early if no neighbours to process
        return;
    }

    node = thisDepot;
    node = node->next;
    for (int i = 0; i < neighbourCount; i++) {
        neighbours[i] = node->name;
        node = node->next;
    }

    qsort(neighbours, neighbourCount, sizeof(char*), string_compare);
    for (int i = 0; i < neighbourCount; i++) {
        printf("%s\n", neighbours[i]);
    }

    fflush(stdout);
    free(goodNames);
    free(neighbours);
}

/**
 * Outputs error messages detected in main thread through
 * stderr.
 *
 * @param status: integer status indicating which error occurred.
 */
void display_err(int status) {

    switch (status) {
        case NUM_ARG_ERR:
            fprintf(stderr, "Usage: 2310depot name {goods qty}\n");
            break;

        case NAME_ERR:
            fprintf(stderr, "Invalid name(s)\n");
            break;

        case QUANTITY_ERR:
            fprintf(stderr, "Invalid quantity\n");
            break;

        default:
            return;
    }
}

/**
 * Checks whether command line args to 2310depot are valid, and reports
 * errors to main. A depot name must be given, and all depot/resource names
 * cannot contain " \n\r:" characters. Quantities must be non-negative
 * numbers.
 *
 * @param argc: the number of command line arguments
 * @param argv: the command line arguments to check
 * @return integer error status to be handled by main if detected, or
 *      0 otherwise.
 */
int check_args(int argc, char* argv[]) {

    if (argc < MIN_ARGS) { // num args check
        return NUM_ARG_ERR;
    }

    if (strcmp("", argv[1]) == 0) {
        return NUM_ARG_ERR;
    }

    char* invalidChars = " \n\r:"; // invalid chars check
    if (!check_characters(argv[1], invalidChars)) {
        return NAME_ERR;
    }

    // if this depot has allocated resources, check them
    if (argc > 2) {
        if (strcmp("", argv[2]) == 0) {
            return NUM_ARG_ERR;
        }

        if (argc % 2 != 0) { // resources must be in name:quantity pairs
            return QUANTITY_ERR;
        }

        for (int i = 2; i < argc; i++) {
            if (i % 2 == 0) {
                if (strcmp("", argv[i]) == 0) {
                    return NAME_ERR;
                }
                if (!check_characters(argv[i], invalidChars)) {
                    return NAME_ERR;
                }

            } else {
                if (strcmp("", argv[i]) == 0) {
                    return QUANTITY_ERR;
                }
                if (!is_a_number(argv[i])) {
                    return QUANTITY_ERR;
                }
                if (atoi(argv[i]) < 0) {
                    return QUANTITY_ERR;
                }
            }
        }
    }
    return 0;
}

/**
 * Parses from the commandline, this depot's name, and any resource
 * names and their quantities, if given, into relevant linkedList structs.
 *
 * @param argc: the number of command line args
 * @param argv: the command line args
 * @param thisDepot: the first depot in the list (this one)
 * @param firstResource: the first resource in the list
 */
void set_args(int argc, char* argv[], struct LinkedList* thisDepot,
        struct LinkedList* firstResource, struct LinkedList* firstDeferral) {

    // set depot name and thread safety info
    thisDepot->name = argv[1];
    firstResource->name = "XXXX";
    firstResource->type.resource.quantity = 0;

    firstDeferral->type.deferral.executed = true;
    firstDeferral->type.deferral.key = -1;
    firstDeferral->name = "first";

    // check resources have been defined
    if (argc < 3) {
        return;
    }

    // set name and quantity for the first resource
    firstResource->name = argv[2];
    firstResource->type.resource.quantity = atoi(argv[3]);

    // create new list entries and assign values for all other resources
    // (start at 4th arg for first additional resource)
    struct LinkedList* newResource;
    for (int i = 4; i < argc; i++) {

        if (i % 2 == 0) {
            // add new resource to list
            newResource = add_item(firstResource);

            // set name
            newResource->name = argv[i];

        } else {
            // set quantity
            newResource->type.resource.quantity = atoi(argv[i]);

        }
    }

}

int main(int argc, char* argv[]) {

    struct sigaction sighup; // setup signal handler - SIGHUP
    sighup.sa_handler = handle_sighup;
    sighup.sa_flags = SA_RESTART;
    sigaction(SIGHUP, &sighup, 0);

    struct sigaction sigpipe; // setup signal handler - SIGPIPE;
    sigpipe.sa_handler = handle_sigpipe;
    sigpipe.sa_flags = SA_RESTART;
    sigaction(SIGPIPE, &sigpipe, 0);

    int err = 0;
    err = check_args(argc, argv); // check args
    if (err) {
        display_err(err);
        return err;
    }

    // setup linked lists for resources, depots and deferred commands
    struct LinkedList* firstResource = malloc(sizeof(struct LinkedList));
    struct LinkedList* thisDepot = malloc(sizeof(struct LinkedList));
    struct LinkedList* firstDeferral = malloc(sizeof(struct LinkedList));

    // instantiate mutex and set args
    pthread_mutex_t dataLock;
    pthread_mutex_init(&dataLock, NULL);
    set_args(argc, argv, thisDepot, firstResource, firstDeferral);

    // start server - listen on ephemeral port
    pthread_t serverTid = start_server(thisDepot, firstResource,
            firstDeferral, &dataLock);

    while (1) { // main thread used to detect signals
        if (sigHupDetected) {
            pthread_mutex_lock(&dataLock);
            display_depot_data(thisDepot, firstResource);
            pthread_mutex_unlock(&dataLock);
            sigHupDetected = false;
        }
    }
    void* result;
    pthread_join(serverTid, &result);
    pthread_mutex_destroy(&dataLock);
    free_linked_list(firstResource);
    free_linked_list(thisDepot);
    free_linked_list(firstDeferral);
    return 0;
}