#ifndef UTIL_H
#define UTIL_H

#include <stdbool.h>
#include <string.h>
#include <ctype.h>

int count_symbol(char* string, char symbol);

int string_compare(const void* v1, const void* v2);

bool check_string_match(char* string, char* msg);

bool check_characters(char* string, char* invalidChars);

bool is_a_number(char* arg);

#endif //UTIL_H
