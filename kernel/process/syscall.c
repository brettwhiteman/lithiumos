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
				if(!vmmngr_alloc_page(stk->ebx + i * PAGE_SIZE))
				{
					stk->eax = (uint32_t)FALSE;

					break;
				}
			}

			break;
	}
}
