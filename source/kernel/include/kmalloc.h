#ifndef KMALLOC_H
#define KMALLOC_H

#include <stdinc.h>

bool kmalloc_init(void);
void *kmalloc(size_t bytes);
void kfree(void *address);

#endif
