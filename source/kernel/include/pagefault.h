#ifndef PAGEFAULT_H
#define PAGEFAULT_H

#include "stdinc.h"
#include "interrupt.h"

void pagefault_handle(isr_t *stk);

#endif
