#ifndef PROCESS_H
#define PROCESS_H

#include <stdinc.h>

typedef struct
{
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t eip, cs, eflags, useresp, ss;
} registers_t;

typedef struct pmem_region_struct
{
	uint32_t pageIndex;
	struct pmem_region_struct *next;
} pmem_region_t;

typedef struct thread_struct
{
	registers_t regs;
	uint32_t entryPoint;
	uint32_t id;
	struct thread_struct *next;
	pmem_region_t *pmemRegions;
} thread_t;

typedef struct process_struct
{
	thread_t *threads;
	thread_t *blockedThreads;
	uint32_t pdPhysical;
	struct process_struct *next;
	void *loadBinaryFrom;
	size_t binarySize;
	uint32_t id;
	uint32_t threadIDCounter;
	pmem_region_t *pmemRegions;
} process_t;

process_t *add_process(void *binary, size_t binarySize);
thread_t *add_thread(process_t *proc, uint32_t entryPoint);
uint32_t setup_process(process_t *proc, uint32_t *entryPoint);
uint32_t init_thread_stack(thread_t *thread, uint32_t *pStackAddr);
void process_add_pmem_region(process_t *proc, uint32_t pageIndex);
void thread_add_pmem_region(thread_t *thread, uint32_t pageIndex);
void process_destroy(process_t *proc);
process_t *add_kernel_process(void *entry);

#endif