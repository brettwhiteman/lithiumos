#include <process.h>
#include <vmmngr.h>
#include <errorcodes.h>
#include <elf.h>
#include <kmalloc.h>

// Stack size must be a multiple of PAGE_SIZE
#define STACK_SIZE 				PAGE_SIZE * 2
#define PAGEDIR_TEMP 			0xFFBFF000

uint32_t idCounter = 0;

process_t *add_process(void *binary, size_t binarySize)
{
	// Setup paging structures
	physical_addr pdPhysical = (physical_addr)pmmngr_alloc_block();

	if(pdPhysical == 0)
		return NULL;

	if(!vmmngr_map_page(pdPhysical, PAGEDIR_TEMP))
	{
		pmmngr_free_block(pdPhysical);

		return NULL;
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
	proc->threads->pmemRegions = NULL;
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
	proc->loadBinaryFrom = binary;
	proc->binarySize = binarySize;
	proc->id = ++idCounter;
	proc->pmemRegions = NULL;

	return proc;
}

thread_t *add_thread(process_t *proc, uint32_t entryPoint)
{
	thread_t *newThread = (thread_t *)kmalloc(sizeof(thread_t));

	newThread->next = NULL;

	newThread->id = ++(proc->threadIDCounter);

	newThread->entryPoint = entryPoint;

	newThread->regs.eip = 0xDEADBEEF;
	newThread->regs.cs = 0x1B; // User code selector ring 3
	newThread->regs.ds = 0x23; // User data selector ring 3
	newThread->regs.es = newThread->regs.ds;
	newThread->regs.fs = newThread->regs.ds;
	newThread->regs.gs = newThread->regs.ds;
	newThread->regs.ss = newThread->regs.ds;
	newThread->regs.eflags = 0x202; // Standard EFLAGS

	return newThread;
}

uint32_t setup_process(process_t *proc, uint32_t *entryPoint)
{
	// Parse ELF
	void *elf = proc->loadBinaryFrom;

	struct elf_load_info eli;

	if(elf32_parse(elf, &eli))
		return ERR_INVALID_ELF_EXECUTABLE;

	*entryPoint = eli.entryPoint;

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

			physical_addr phys = vmmngr_get_physical_address(addr);

			if(phys == NULL)
				return ERR_UNKNOWN;

			process_add_pmem_region(proc, phys);

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

	return SUCCESS;
}

uint32_t init_thread_stack(thread_t *thread, uint32_t *pStackAddr)
{
	// Threads have stacks based on their id (minimum id is 1)
	virtual_addr stackAddr = 0xC0000000 - STACK_SIZE * thread->id;

	for(uint32_t addr = stackAddr; addr < stackAddr + STACK_SIZE; addr += PAGE_SIZE)
	{
		if(!vmmngr_alloc_page((virtual_addr)addr))
			return ERR_OUT_OF_MEMORY;

		physical_addr phys = vmmngr_get_physical_address((virtual_addr)addr);

		if(phys == NULL)
			return ERR_UNKNOWN;

		thread_add_pmem_region(thread, (uint32_t)phys / PAGE_SIZE);

		pd_entry *pde = (pd_entry *)(PAGE_DIRECTORY_ADDRESS + PAGE_DIRECTORY_INDEX(addr) * sizeof(pd_entry));
		pd_entry_add_attrib(pde, PDE_USER);

		pt_entry *pte = (pt_entry *)((uint32_t)vmmngr_get_ptable_address(addr) + PAGE_TABLE_INDEX(addr) * sizeof(pt_entry));
		pt_entry_add_attrib(pte, PTE_USER);

		vmmngr_flush_tlb_entry(addr);
	}

	*pStackAddr = stackAddr + STACK_SIZE;

	return SUCCESS;
}

void process_add_pmem_region(process_t *proc, uint32_t pageIndex)
{
	if(proc->pmemRegions == NULL)
	{
		proc->pmemRegions = (pmem_region_t *)kmalloc(sizeof(pmem_region_t));
		proc->pmemRegions->pageIndex = pageIndex;
		proc->pmemRegions->next = NULL;
	}
	else
	{
		pmem_region_t *region = proc->pmemRegions;

		while(region->next != NULL)
			region = region->next;

		region->next = (pmem_region_t *)kmalloc(sizeof(pmem_region_t));

		region = region->next;

		region->pageIndex = pageIndex;

		region->next = NULL;
	}
}

void thread_add_pmem_region(thread_t *thread, uint32_t pageIndex)
{
	if(thread->pmemRegions == NULL)
	{
		thread->pmemRegions = (pmem_region_t *)kmalloc(sizeof(pmem_region_t));
		thread->pmemRegions->pageIndex = pageIndex;
		thread->pmemRegions->next = NULL;
	}
	else
	{
		pmem_region_t *region = thread->pmemRegions;

		while(region->next != NULL)
			region = region->next;

		region->next = (pmem_region_t *)kmalloc(sizeof(pmem_region_t));

		region = region->next;

		region->pageIndex = pageIndex;

		region->next = NULL;
	}
}