#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdinc.h>
#include <vmmngr.h>
#include <interrupt.h>

typedef struct registers_struct
{
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t eip, cs, eflags, useresp, ss;
} registers_t;

typedef struct thread_struct
{
	registers_t regs;
	void *entryPoint;
	uint32_t id;
	struct thread_struct *next;
} thread_t;

typedef struct process_struct
{
	thread_t *threads;
	thread_t *blockedThreads;
	physical_addr pdPhysical;
	struct process_struct *next;
	void *loadBinaryFrom;
	size_t binarySize;
	uint32_t id;
	uint32_t threadIDCounter;
} process_t;

void scheduler_tick(registers_t *regs);
void scheduler_switch_process(registers_t *regs);
uint32_t scheduler_add_process(void *procBinary, size_t procBinarySize);
void scheduler_setup_tss(void);
uint32_t scheduler_setup_current_thread(isr_t *stk);
uint32_t scheduler_add_thread(uint32_t procID, void *entryPoint);

#endif
