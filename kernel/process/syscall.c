#include <syscall.h>
#include <print.h>
#include <stdinc.h>
#include <vmmngr.h>

#define SYSCALL_PRINT 			0
#define SYSCALL_VIRTUAL_ALLOC 	1

void call_handler(isr_t *stk)
{
	switch(stk->eax)
	{
		case SYSCALL_PRINT:
			print_string((const char *)stk->ebx);
			
			break;

		case SYSCALL_VIRTUAL_ALLOC:
			// EBX: Start address
			// ECX: Number of pages to map

			stk->eax = (uint32_t)TRUE;

			for(uint32_t i = 0; i < stk->ecx; ++i)
			{
				virtual_addr addr = stk->ebx + i * PAGE_SIZE;

				if(!vmmngr_alloc_page(addr))
				{
					stk->eax = (uint32_t)FALSE;

					break;
				}

				pd_entry *pde = (pd_entry *)(PAGE_DIRECTORY_ADDRESS + PAGE_DIRECTORY_INDEX(addr) * sizeof(pd_entry));
				pd_entry_add_attrib(pde, PDE_USER);

				pt_entry *pte = (pt_entry *)((uint32_t)vmmngr_get_ptable_address(addr) +
					PAGE_TABLE_INDEX(addr) * sizeof(pt_entry));

				pt_entry_add_attrib(pte, PTE_USER);
			}

			break;
	}
}
