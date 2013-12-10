#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <interrupt.h>

void keyboard_handler(isr_t *stk);
void keyboard_install(void);
char kgetch_blocking(void);

#endif
