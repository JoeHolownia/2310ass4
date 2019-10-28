#include "messaging.h"
#include "util.h"
#include "linkedLists.h"

#define MAX_CONNECT_MSG_SIZE 13
#define MIN_IM_MSG_SIZE 6
#define DELIVER 0
#define WITHDRAW 1
#define MIN_DEFER_MSG_SIZE 8
#define MIN_EXECUTE_MSG_SIZE 9
#define MIN_TRANSFER_MSG_SIZE 14

/**
 * Checks a received IM message for any errors, and reports
 * these errors to the IM message handler.
 *
 * @param message: the string message to be checked
 * @return true if no errors are detected in the message, false
 *      otherwise. If an error is detected, do not handle the message.
 */
bool check_im_message(char* message) {

    // length check
    if (strlen(message) < MIN_IM_MSG_SIZE) {
        return false;
    }

    // IM check
    char* im = "IM";
    if (!check_string_match(im, message)) {
        return false;
    }

    // Correct number of ':' symbol check
    if (count_symbol(message, ':') != 2) {
        return false;
    }

    char* checkMessage = malloc(sizeof(char) * strlen(message));
    strcpy(checkMessage, message);

    char* port;
    char* name;
    strtok_r(checkMessage, ":", &checkMessage);
    port = strtok_r(checkMessage, ":", &checkMessage);
    name = strtok_r(checkMessage, ":", &checkMessage);

    // check port is a number
    if (!is_a_number(port)) {
        return false;
    }

    // check name isn't invalid
    char* invalid = " \n\r:";
    if (!check_characters(name, invalid)) {
        return false;
    }

    return true;
}

/**
 * Checks a received connect message for any errors, and reports
 * these to the connect message handler.
 *
 * @param message: the string message to be checked
 * @return true if no errors are detected in the message, false
 *      otherwise. If an error is detected, do not handle the message.
 */
bool check_connect_message(char* message) {

    // length check
    if (strlen(message) != MAX_CONNECT_MSG_SIZE) {
        return false;
    }

    // check for "Connect"
    char* connect = "Connect";
    if (!check_string_match(connect, message)) {
        return false;
    }

    // check for correct number of ':' symbols (1)
    if (count_symbol(message, ':') != 1) {
        return false;
    }

    char* checkMessage = malloc(sizeof(char) * strlen(message));
    strcpy(checkMessage, message);

    char* port;
    strtok_r(checkMessage, ":", &checkMessage);
    port = strtok_r(checkMessage, ":", &checkMessage);

    // check port is a number
    if (!is_a_number(port)) {
        return false;
    }

    return true;
}

/**
 * Checks a received deliver or withdraw message for any errors, and reports
 * these to the deliver/withdraw message handler.
 *
 * @param message: the string message to be checked
 * @return true if no errors are detected in the message, false
 *      otherwise. If an error is detected, do not handle the message.
 */
bool check_deliver_withdraw_message(char* message, char* commandString) {

    // length check (+4 for :q:t)
    if (strlen(message) < strlen(commandString) + 4) {
        return false;
    }

    // "Deliver" or "Withdraw" check
    if (!check_string_match(commandString, message)) {
        return false;
    }

    // Correct count of ':' symbol check
    if (count_symbol(message, ':') != 2) {
        return false;
    }

    // break down message
    char* checkMessage = malloc(sizeof(char) * strlen(message));
    strcpy(checkMessage, message);

    char* quantity;
    char* type;
    strtok_r(checkMessage, ":", &checkMessage);
    quantity = strtok_r(checkMessage, ":", &checkMessage);
    type = strtok_r(checkMessage, ":", &checkMessage);

    // check quantity is valid number
    if (!is_a_number(quantity)) {
        return false;
    }

    // check quantity is above 0
    if (atoi(quantity) <= 0) {
        return false;
    }

    // check type has no invalid chars
    char* invalid = " \n\r:";
    if (!check_characters(type, invalid)) {
        return false;
    }

    return true;
}

/**
 * Message handler for a received Deliver or Withdraw message. First
 * checks the message is valid, then finds the resource given by
 * type (t) in: Deliver:q:t or Withdraw:q:t in the depot's current list.
 * If it is a Deliver message, add the quantity to the given type. If it
 * is a Withdraw message, subtract the quantity from the given type. If the
 * type does not exist create a new instance of that type in the resource list,
 * and then perform +/- operations upon it.
 *
 * @param message: the received deliver/withdraw message
 * @param firstResource: the first resource in this depots linked list of
 *      resources
 * @param dataLock: the mutex to lock this depot's resource list with
 * @param command: boolean macro DELIVER or WITHDRAW, treats the incoming
 *      message as a deliver or withdraw message.
 */
void handle_deliver_withdraw_message(char* message,
        struct LinkedList* firstResource, pthread_mutex_t* dataLock,
        int command) {

    char* commandString;
    if (command == DELIVER) {
        commandString = "Deliver";
    } else {
        commandString = "Withdraw";
    }

    // silently ignore faulty deliver/withdraw message
    if(!check_deliver_withdraw_message(message, commandString)) {
        return;
    }

    // split up string
    char* quantity;
    char* type;
    strtok_r(message, ":", &message);
    quantity = strtok_r(message, ":", &message);
    type = strtok_r(message, ":", &message);

    pthread_mutex_lock(dataLock);
    struct LinkedList* resource = search_list_by_name(type, firstResource);

    // create new resource if it doesn't exist already
    if (resource == NULL) {
        resource = add_item(firstResource);
        resource->name = type;
    }

    // decide whether to add/subtract quantity from resource
    if (command == DELIVER) {
        resource->type.resource.quantity += atoi(quantity);

    } else {
        resource->type.resource.quantity -= atoi(quantity);
    }

    pthread_mutex_unlock(dataLock);
}

/**
 * Checks a received transfer message for any errors, and reports
 * these to the transfer message handler.
 *
 * @param message: the string message to be checked
 * @return true if no errors are detected in the message, false
 *      otherwise. If an error is detected, do not handle the message.
 */
bool check_transfer_message(char* message) {

    // length check
    if (strlen(message) < MIN_TRANSFER_MSG_SIZE) {
        return false;
    }

    // check for "Transfer"
    char* transfer = "Transfer";
    if (!check_string_match(transfer, message)) {
        return false;
    }

    // check for correct ':' symbol count
    if (count_symbol(message, ':') != 3) {
        return false;
    }

    return true;
}

/**
 * Message handler for a transfer message of the format: Transfer:q:t:dest,
 * where q is the quantity of the resource, t is the type of resource and
 * dest is the name of the destination depot to transfer to. First checks
 * whether this message is valid, then finds the destination depot, withdraws
 * the given quantity of the resource from the current depot's stocks
 * (given by q and t) and then sends a deliver message to the other depot.
 *
 * @param message: the transfer message to handle
 * @param firstResource: the first resource in this depot's linked list of
 *      resources
 * @param thisDepot: this depot as the first item in a linked list of depots
 * @param dataLock: a mutex to protect both the depot and resource list
 */
void handle_transfer_message(char* message,
        struct LinkedList* firstResource, struct LinkedList* thisDepot,
        pthread_mutex_t* dataLock) {

    // do nothing if message is invalid
    if (!check_transfer_message(message)) {
        return;
    }

    // split up message
    char* quantity;
    char* type;
    char* dest;
    strtok_r(message, ":", &message);
    quantity = strtok_r(message, ":", &message);
    type = strtok_r(message, ":", &message);
    dest = strtok_r(message, ":", &message);

    // cannot transfer to self
    if (dest == thisDepot->name) {
        return;
    }

    // find destination depot for delivery
    pthread_mutex_lock(dataLock);
    struct LinkedList* destination = search_list_by_name(dest, thisDepot);
    if (destination == NULL) {
        return;
    }

    // find resource in current directory, and withdraw quantity
    struct LinkedList* resource = search_list_by_name(type, firstResource);
    if (resource == NULL) {
        resource = add_item(firstResource);
        resource->name = type;
    }
    resource->type.resource.quantity -= atoi(quantity);

    // send deliver message to other depot
    fprintf(destination->type.depot.to, "Deliver:%s:%s\n", quantity, type);
    fflush(destination->type.depot.to);
    pthread_mutex_unlock(dataLock);
}

/**
 * Checks a received defer message for any errors, and reports
 * these to the defer message handler. Only checks the defer
 * portion of the message, as the rest will be checked by
 * the corresponding message handler for the given deferred
 * operation.
 *
 * @param message: the string message to be checked
 * @return true if no errors are detected in the message, false
 *      otherwise. If an error is detected, do not handle the message.
 */
bool check_defer_message(char* message) {

    // length check
    if (strlen(message) < MIN_DEFER_MSG_SIZE) {
        return false;
    }

    // check for "Defer"
    char* defer = "Defer";
    if (!check_string_match(defer, message)) {
        return false;
    }

    // split up string
    char* key;
    char* checkMessage = malloc(sizeof(char) * strlen(message));
    strcpy(checkMessage, message);
    strtok_r(checkMessage, ":", &checkMessage);
    key = strtok_r(checkMessage, ":", &checkMessage);

    // check key is number
    if (!is_a_number(key)) {
        return false;
    }

    // check key is positive
    if (atoi(key) < 0) {
        return false;
    }

    return true;
}

/**
 * Checks a received execute message for any errors, and reports
 * these to the execute message handler.
 *
 * @param message: the string message to be checked
 * @return true if no errors are detected in the message, false
 *      otherwise. If an error is detected, do not handle the message.
 */
bool check_execute_message(char* message) {

    // length check
    if (strlen(message) < MIN_EXECUTE_MSG_SIZE) {
        return false;
    }

    // check for "Execute"
    char* execute = "Execute";
    if (!check_string_match(execute, message)) {
        return false;
    }

    // split up message
    char* key;
    char* checkMessage = malloc(sizeof(char) * strlen(message));
    strcpy(checkMessage, message);
    strtok_r(checkMessage, ":", &checkMessage);
    key = strtok_r(checkMessage, ":", &checkMessage);

    // check key is number
    if (!is_a_number(key)) {
        return false;
    }

    // check key is positive
    if (atoi(key) < 0) {
        return false;
    }

    return true;
}

/**
 * Message handler for execute messages, of the format Execute:k,
 * where k is the key of the operations(s) to execute. First checks
 * the message, then searches for all listed deferrals for this depot
 * with the given key (k). If detected, these deferrals are then executed,
 * and their corresponding operations will be processed by the depot.
 *
 * @param message: the execute message to handle
 * @param firstDeferral: the first in a linked list of this depot's deferred
 *      operations
 * @param dataLock: a mutex to protect this depot's list of deferrals
 */
void handle_execute_message(char* message, struct LinkedList* firstDeferral,
        pthread_mutex_t* dataLock) {

    // check execute message
    if (!check_execute_message(message)) {
        return;
    }

    // digest execute message
    char* key;
    strtok_r(message, ":", &message);
    key = strtok_r(message, ":", &message);

    // execute all deferals with given key
    pthread_mutex_lock(dataLock);
    struct LinkedList* deferral = search_deferrals_by_key(atoi(key),
            firstDeferral);
    while(deferral != NULL) {
        deferral->type.deferral.executed = true;
        deferral->type.deferral.key = -1;
        deferral = search_deferrals_by_key(atoi(key), firstDeferral);
    }
    pthread_mutex_unlock(dataLock);
}




