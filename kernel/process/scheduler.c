/*
Task scheduler for Lithium OS.

28/11/13 - BMW: Created.

29/11/13 - BMW:	Added support for multiple threads per process.
				Each process is now a container for threads.

30/11/13 - BMW:	Modified scheduler_setup_current_thread() so it
				doesn't copy binary or setup paging when it already
				has done it.
*/

#include <scheduler.h>
#include <pmmngr.h>
#include <errorcodes.h>
#include <kmalloc.h>
#include <gdt.h>
#include <pde.h>
#include <pte.h>
#include <elf.h>
#include <vmmngr.h>

#define PAGEDIR_TEMP 			0xFFBFF000

process_t *pQueue = NULL;
process_t *currentProc = NULL;
thread_t *currentThread = NULL;
uint32_t idCounter = 0;

void scheduler_tick(registers_t *regs)
{
	// TODO: more complex switching system (priorities maybe)

	scheduler_switch_process(regs);
}

void scheduler_switch_process(registers_t *regs)
{
	if(pQueue == NULL)
		return; // Scheduling not set up yet

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

	uint32_t pdOffset = PAGE_DIRECTORY_INDEX(0xC0000000) * 4;

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

	// Setup paging structures
	physical_addr pdPhysical = (physical_addr)pmmngr_alloc_block();

	if(pdPhysical == 0)
		return 0;

	if(!vmmngr_map_page(pdPhysical, PAGEDIR_TEMP))
	{
		pmmngr_free_block(pdPhysical);

		return 0;
	}

	vmmngr_flush_tlb_entry(PAGEDIR_TEMP);

	pdirectory *pd = (pdirectory *)PAGEDIR_TEMP;

	// Clear page directory
	memsetd((uint32_t *)pd, 0, sizeof(pdirectory) / 4);

	// Set up process struct and add to queue.
	// Will use 0xDEADBEEF for EIP so the page fault handler will
	// know that it needs to call scheduler_setup_current_thread().
	process_t *proc = (process_t *)kmalloc(sizeof(process_t));

	proc->threadIDCounter = 0;
	proc->threads = (thread_t *)kmalloc(sizeof(thread_t));
	proc->threads->next = NULL;
	proc->threads->id = ++(proc->threadIDCounter);
	proc->blockedThreads = NULL;

	registers_t *pRegs = &proc->threads->regs;

	pRegs->eip = 0xDEADBEEF;
	pRegs->cs = 0x1B; // User code selector ring 3
	pRegs->ds = 0x23; // User data selector ring 3
	pRegs->es = pRegs->ds;
	pRegs->fs = pRegs->ds;
	pRegs->gs = pRegs->ds;
	pRegs->ss = pRegs->ds;
	pRegs->eflags = 0x202; // Standard EFLAGS

	proc->pdPhysical = pdPhysical;
	proc->next = NULL;
	proc->loadBinaryFrom = procBinary;
	proc->binarySize = procBinarySize;
	proc->id = ++idCounter;

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

void scheduler_setup_tss(void)
{
	void *stack = kmalloc(2048);

	void *tss = kmalloc(104);

	memset(tss, 0, 104);

	*(uint32_t *)((uint32_t)tss + 4) = (uint32_t)stack + 1024; // Stack grows down so add 1024 to start at top
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
	// Only set up paging and copy binary if we haven't already
	if(currentProc->binarySize != 0)
	{
		// Parse ELF
		void *elf = currentProc->loadBinaryFrom;

		struct elf_load_info eli;

		if(elf32_parse(elf, &eli))
			return ERR_INVALID_ELF_EXECUTABLE;

		stk->eip = eli.entryPoint;

		for(uint32_t i = 0; i < eli.numSegs; ++i)
		{
			uint32_t sizeToMap = eli.segs[i].sizeInMemory + PAGE_SIZE;

			if(sizeToMap & (PAGE_SIZE - 1))
			{
				sizeToMap += PAGE_SIZE;
				sizeToMap &= (uint32_t)(~(PAGE_SIZE - 1));
			}

			for(uint32_t j = 0; j < sizeToMap; j += PAGE_SIZE)
			{
				virtual_addr addr = eli.segs[i].addressInMemory + j;

				if(!vmmngr_alloc_page(addr))
					return ERR_OUT_OF_MEMORY;

				pd_entry *pde = vmmngr_pdirectory_lookup_entry((pdirectory *)PAGE_DIRECTORY_ADDRESS, addr);
				pd_entry_add_attrib(pde, PDE_USER);

				pt_entry *pte = (pt_entry *)((uint32_t)vmmngr_get_ptable_address(addr)
					+ PAGE_TABLE_INDEX(addr) * 4);
				pt_entry_add_attrib(pte, PTE_USER);
			}

			// Copy segment to correct location
			memcpy((void *)eli.segs[i].addressInMemory, (const void *)((uint32_t)elf + eli.segs[i].offsetInFile),
				eli.segs[i].sizeInFile);

			// Zero any extra memory space
			uint32_t zeroSpace = eli.segs[i].sizeInMemory - eli.segs[i].sizeInFile;

			if(zeroSpace > 0)
				memset((uint8_t *)(eli.segs[i].addressInMemory + eli.segs[i].sizeInFile), 0x00, zeroSpace);
		}

		// So we know we have set it up
		currentProc->binarySize = 0;
	}

	// Threads have stacks based on their id (minimum id is 1)
	virtual_addr stackLoc = 0xC0000000 - 0x1000 * currentThread->id;

	if(!vmmngr_alloc_page((virtual_addr)stackLoc))
		return ERR_OUT_OF_MEMORY;

	pd_entry *pde = (pd_entry *)(PAGE_DIRECTORY_ADDRESS + PAGE_DIRECTORY_INDEX(stackLoc) * 4);

	pd_entry_add_attrib(pde, PDE_USER);

	pt_entry *pte = (pt_entry *)((uint32_t)vmmngr_get_ptable_address(stackLoc) + PAGE_TABLE_INDEX(stackLoc) * 4);

	pt_entry_add_attrib(pte, PTE_USER);

	vmmngr_flush_tlb_entry(stackLoc);

	// Set stack pointer
	stk->useresp = (uint32_t)stackLoc + 0x1000;

	return SUCCESS;
}

uint32_t scheduler_add_thread(uint32_t procID, void *entryPoint)
{
	process_t *p = pQueue;

	// Find process with id procID
	while(p != NULL)
	{
		if(p->id == procID)
			break;

		p = p->next;
	}

	if(p == NULL)
		return 0;

	thread_t *t = p->threads;

	while(t->next != NULL)
	{
		t = t->next;
	}

	thread_t *newThread = (thread_t *)kmalloc(sizeof(thread_t));

	newThread->next = NULL;

	newThread->id = ++(p->threadIDCounter);

	newThread->entryPoint = entryPoint;

	newThread->regs.eip = 0xDEADBEEF;
	newThread->regs.cs = 0x1B; // User code selector ring 3
	newThread->regs.ds = 0x23; // User data selector ring 3
	newThread->regs.es = newThread->regs.ds;
	newThread->regs.fs = newThread->regs.ds;
	newThread->regs.gs = newThread->regs.ds;
	newThread->regs.ss = newThread->regs.ds;
	newThread->regs.eflags = 0x202; // Standard EFLAGS

	t->next = newThread;

	return newThread->id;
}
