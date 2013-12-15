#ifndef STRING_H
#define STRING_H

#include <types.h>

size_t strlen(const char *str);
void strrev(char *str);
int strcmp(const char *str1, const char *str2);
char *strtok(char *str, const char *delimiters, char **context);

#endif