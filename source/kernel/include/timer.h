#ifndef TIMER_H
#define TIMER_H

#include "stdinc.h"
#include "interrupt.h"

void set_timer_frequency(uint32_t hz);
void timer_handler(isr_t *stk);
void timer_install(void);
void timer_wait(uint32_t ticks);
uint32_t get_tick_count(void);

#endif
