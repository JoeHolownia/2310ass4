#include "util.h"

/**
 * Counts and returns the number of a specific symbol in
 * a string.
 *
 * @parame string: the string to be checked
 * @param symbol: the symbol to count
 * @return: integer count of instances of symbol in the given string
 *
 */
int count_symbol(char* string, char symbol) {

    int numSymbol = 0;
    for (int i = 0; i < strlen(string); i++) {
        if (string[i] == symbol) {
            numSymbol++;
        }
    }
    return numSymbol;
}

/**
 * Compare function for qsort for displaying of depot resources/neighbours.
 * Two strings are compared, and their order given to qsort lexicographically.
 *
 * @param v1: the first string to compare
 * @param v2: the second string to compare
 * @return (-) integer, if qsort is to move this entry in the list left,
 *      (+)  integer, if qsort is to move this entry in the list right or
 *      0 if the item is in the correct place.
 */
int string_compare(const void* v1, const void* v2) {

    char* string1 = *((char**)v1);
    char* string2 = *((char**)v2);

    return strcmp(string1, string2);

}

/**
 * Helper function which checks whether a given message matches
 * a specific string, useful for error checking messages.
 *
 * @param string: the string to check against
 * @param msg: the message being checked
 * @return true if string matches, false otherwise.
 */
bool check_string_match(char* string, char* msg) {

    if (strlen(msg) < strlen(string)) {
        return false;
    }

    for (int i = 0; i < strlen(string); i++) {

        if (msg[i] != string[i]) {
            return false;
        }
    }

    return true;
}

/**
 * Check for presence of specific invalid characters in a string. If
 * any of the invalid characters are detected, return false.
 *
 * @param string: the string to check for invalid characters
 * @param invalidChars: the characters that are not allowed in the string
 * @return true if none of the invalid characters are found in the string,
 *      false if an invalid character exists
 */
bool check_characters(char* string, char* invalidChars) {

    for (int i = 0; i < strlen(string); i++) {

        for (int j = 0; j < strlen(invalidChars); j++) {

            if (string[i] == invalidChars[j]) {
                return false;
            }

        }
    }

    return true;
}

/**
 * Helper function for arg checking, checks if
 * all chars in the given argument are digits.
 *
 * @param arg: string argument to check
 * @return true if all chars are digits, false otherwise
 */
bool is_a_number(char* arg) {

    for (int i = 0; i < strlen(arg); i++) {
        if (!isdigit(arg[i])) {
            return false;
        }
    }

    return true;
}