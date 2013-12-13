#ifndef MALLOC_H
#define MALLOC_H

#include <types.h>

bool malloc_init(void);
void *malloc(size_t bytes);
void free(void *mem);

#endif
