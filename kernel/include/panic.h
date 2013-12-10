#ifndef PANIC_H
#define PANIC_H

#include <interrupt.h>

void panic_display_message(isr_t *stk);

#endif
