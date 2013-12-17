/*
Task scheduler for Lithium OS.
*/

#include <scheduler.h>
#include <errorcodes.h>
#include <kmalloc.h>
#include <gdt.h>
#include <pde.h>
#include <vmmngr.h>
#include <print.h>

#define KERNEL_STACK_ADDRESS	0xF0002000
#define PAGEDIR_TEMP 			0xFFBFF000

process_t *pQueue = NULL;
process_t *currentProc = NULL;
process_t *previousProc = NULL;
thread_t *currentThread = NULL;

void scheduler_tick(registers_t *regs)
{
	// TODO: more complex switching system (priorities maybe)

	scheduler_switch_process(regs);
}

void scheduler_switch_process(registers_t *regs)
{
	if(pQueue == NULL)
		return; // No processes!

	if(currentProc == NULL)
	{
		// Must be first process, get it off start of queue
		currentProc = pQueue;

		// Get first thread
		currentThread = currentProc->threads;
	}
	else
	{
		// Save current registers
		memcpy(&currentThread->regs, regs, sizeof(registers_t));

		if(currentThread->next == NULL)
		{
			// Switch to next process

			previousProc = currentProc;

			if(currentProc->next == NULL)
			{
				// Go back to start of queue
				currentProc = pQueue;

				// And get first thread
				currentThread = currentProc->threads;
			}
			else
			{
				// Switch to next process in queue
				currentProc = currentProc->next;

				// And get first thread
				currentThread = currentProc->threads;
			}
		}
		else
		{
			// Switch to next thread
			currentThread = currentThread->next;
		}
	}

	memcpy(regs, &currentThread->regs, sizeof(registers_t));

	vmmngr_map_page(currentProc->pdPhysical, PAGEDIR_TEMP);
	vmmngr_flush_tlb_entry(PAGEDIR_TEMP);

	uint32_t pdOffset = PAGE_DIRECTORY_INDEX(0xC0000000) * sizeof(pd_entry);

	// Copy kernel address space
	memcpy((void *)(PAGEDIR_TEMP + pdOffset), (void *)(PAGE_DIRECTORY_ADDRESS + pdOffset),
		PAGE_DIRECTORY_SIZE - pdOffset);

	// Update physical address of page directory to match new process (recursive paging)
	pd_entry *pde = vmmngr_pdirectory_lookup_entry((pdirectory *)PAGEDIR_TEMP, PAGE_DIRECTORY_ADDRESS);
	pd_entry_set_frame(pde, currentProc->pdPhysical);

	vmmngr_switch_pdirectory(currentProc->pdPhysical);
}

uint32_t scheduler_add_process(void *procBinary, size_t procBinarySize)
{
	if((procBinarySize == 0) || (procBinary == NULL))
		return 0;

	process_t *newProc = add_process(procBinary, procBinarySize);

	if(newProc == NULL)
		return 0;

	process_t *p = pQueue;

	if(p == NULL)
		pQueue = newProc;
	else
	{
		while(p->next != NULL)
			p = p->next;

		p->next = newProc;
	}

	return newProc->id;
}

void scheduler_setup_tss(void)
{
	uint32_t stackAddr = KERNEL_STACK_ADDRESS;

	void *tss = kmalloc(104);

	memset(tss, 0, 104);

	*(uint32_t *)((uint32_t)tss + 4) = stackAddr; // Stack grows down so add 1024 to start at top
	*(uint32_t *)((uint32_t)tss + 8) = 0x10; // Kernel data descriptor

	gdt_set_entry(5, create_gdt_entry((uint32_t)tss + 104, (uint32_t)tss, 0x89, 0x4));

	load_gdt(6, FALSE);
	
	__asm__ __volatile__("ltr %%ax" : : "a" (0x2B)); // Selector is 0x28 with RPL 3 so 0x2B
}

/*
Called upon a page fault with 0xDEADBEEF as EIP
*/
uint32_t scheduler_setup_current_thread(isr_t *stk)
{
	uint32_t ret = 0;

	// Only set up paging and copy binary if we haven't already
	if(currentProc->binarySize != 0)
	{
		ret = setup_process(currentProc, &stk->eip);
		
		if(ret)
			return ret;

		// So we know we have set it up
		currentProc->binarySize = 0;
	}

	ret = init_thread_stack(currentThread, &stk->useresp);

	if(ret)
		return ret;

	return SUCCESS;
}

uint32_t scheduler_add_thread(uint32_t procID, uint32_t entryPoint)
{
	process_t *proc = pQueue;

	// Find process with id procID
	while(proc != NULL)
	{
		if(proc->id == procID)
			break;

		proc = proc->next;
	}

	if(proc == NULL)
		return 0;

	thread_t *thread = proc->threads;

	while(thread->next != NULL)
		thread = thread->next;

	thread->next = add_thread(proc, entryPoint);

	return thread->next->id;
}

void scheduler_remove_current_process(isr_t *stk)
{
	process_t *procToRemove = currentProc;
	uint32_t pdPhysical = procToRemove->pdPhysical;

	previousProc->next = procToRemove->next;

	registers_t regs;
	regs.gs = stk->gs;
	regs.fs = stk->fs;
	regs.es = stk->es;
	regs.ds = stk->ds;
	regs.edi = stk->edi;
	regs.esi = stk->esi;
	regs.ebp = stk->ebp;
	regs.esp = stk->esp;
	regs.ebx = stk->ebx;
	regs.edx = stk->edx;
	regs.ecx = stk->ecx;
	regs.eax = stk->eax;
	regs.eip = stk->eip;
	regs.cs = stk->cs;
	regs.eflags = stk->eflags;
	regs.useresp = stk->useresp;
	regs.ss = stk->ss;

	// This will setup the next process so that when we return, it will return
	// to the next process, and it will also allow us to free the page directory
	// since we will be using the next process's page directory.
	scheduler_switch_process(&regs);

	stk->gs = regs.gs;
	stk->fs = regs.fs;
	stk->es = regs.es;
	stk->ds = regs.ds;
	stk->edi = regs.edi;
	stk->esi = regs.esi;
	stk->ebp = regs.ebp;
	stk->ebx = regs.ebx;
	stk->edx = regs.edx;
	stk->ecx = regs.ecx;
	stk->eax = regs.eax;
	stk->eip = regs.eip;
	stk->cs = regs.cs;
	stk->eflags = regs.eflags;
	stk->useresp = regs.useresp;
	stk->ss = regs.ss;

	process_destroy(procToRemove);

	pmmngr_free_block(pdPhysical);
}

uint32_t scheduler_add_kernel_process(void *entry)
{
	process_t *proc = add_kernel_process(entry);

	process_t *p = pQueue;

	if(p == NULL)
		pQueue = proc;
	else
	{
		while(p->next != NULL)
			p = p->next;

		p->next = proc;
	}

	return proc->id;
}
