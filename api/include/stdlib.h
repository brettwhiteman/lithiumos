#ifndef STDLIB_H
#define STDLIB_H

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

#include <types.h>

char *itoa(int value, char *str, int base);
bool malloc_init(void);
void *malloc(size_t bytes);
void free(void *mem);
void exit(int status);

#endif