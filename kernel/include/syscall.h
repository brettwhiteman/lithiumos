#ifndef SYSCALL_H
#define SYSCALL_H

#include <interrupt.h>

void call_handler(isr_t *stk);

#endif
