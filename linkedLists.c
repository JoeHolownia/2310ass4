#include "linkedLists.h"

/**
 * Searches a deferred list for a deferred operation with a sppecific key,
 * and returns a pointer to that deferral
 *
 * @param key: the deferral key to search for
 * @param firstItem: the first deferral in the list
 * @return a pointer to the deferral found in the list, or null if
 *      nothing found
 */
struct LinkedList* search_deferrals_by_key(int key,
        struct LinkedList* firstDeferral) {

    struct LinkedList* searchedDeferral;
    struct LinkedList* node = firstDeferral;
    while (node != NULL) { // process all nodes

        if (key == node->type.deferral.key) {
            searchedDeferral = node;
            return searchedDeferral;
        }
        node = node->next;
    }

    return NULL;
}

/**
 * Counts the number of items in a given linked list.
 * @param first: the first item in the linked list
 * @return an integer number of items that have been counted in the list
 */
int count_items_in_list(struct LinkedList* first) {

    int count = 0;

    struct LinkedList* node = first;
    while (node != NULL) { // process all nodes
        count += 1;
        node = node->next;
    }

    return count;
}

/**
 * Adds a new item to the end of a given linked list, works like
 * a stack "push". In more detail, gives the last item in the
 * current list a pointer to a new item to be added, then sets the
 * new item's pointer to the next item to NULL (as it is last).
 *
 * @param first: the first item in the linked list to be added to
 * @return a pointer reference to the new item in the linked list,
 *      so it may be directly accessed after creation
 */
struct LinkedList* add_item(struct LinkedList* first) {

    struct LinkedList* current = first;

    // iterate through list to get last element
    while (current->next != NULL) {
        current = current->next;
    }

    current->next = malloc(sizeof(struct LinkedList));

    // set next item in list to NULL (end of list)
    current->next->next = NULL;

    return current->next;
}

/**
 * Searches a linked list for a specific item with a given name, and returns a
 * pointer to that item.
 *
 * @param search: the name of the item to search for
 * @param firstItem: the first item in the linked list
 * @return a pointer to the item found in the list, or null if nothing found
 */
struct LinkedList* search_list_by_name(char* search,
        struct LinkedList* firstItem) {

    struct LinkedList* searchedItem;
    struct LinkedList* node = firstItem;
    while (node != NULL) { // process all nodes

        if (strcmp(node->name, search) == 0) {
            searchedItem = node;
            // if found, return the list item
            return searchedItem;
        }
        node = node->next;
    }

    return NULL;
}

/**
 * Handles memory freeing of a linked list, by iterating through all
 * elements and freeing them.
 * @param first: the first item in the linked list being freed
 */
void free_linked_list(struct LinkedList* first) {

    struct LinkedList* node;

    while (first != NULL) {
        node = first;
        first = node->next;
        free(node);
    }
}

