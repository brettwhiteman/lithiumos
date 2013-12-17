#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdinc.h>
#include <interrupt.h>
#include <process.h>

void scheduler_tick(registers_t *regs);
void scheduler_switch_process(registers_t *regs);
uint32_t scheduler_add_process(void *procBinary, size_t procBinarySize);
void scheduler_setup_tss(void);
uint32_t scheduler_setup_current_thread(isr_t *stk);
uint32_t scheduler_add_thread(uint32_t procID, uint32_t entryPoint);
void scheduler_remove_current_process(isr_t *stk);
uint32_t scheduler_add_kernel_process(void *entry);

#endif
