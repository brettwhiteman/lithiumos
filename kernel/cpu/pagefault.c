/*
Lithium OS page fault handler
*/

#include <pagefault.h>
#include <scheduler.h>
#include <panic.h>

/*
Handles page fault exceptions (called from fault handler).
*/
void pagefault_handle(isr_t *stk)
{
	if(stk->eip == 0xDEADBEEF)
	{
		// Init new thread's virtual address space
		if(scheduler_setup_current_thread(stk))
		{
			// Error
			panic_display_message(stk);
		}
	}
	else
		panic_display_message(stk);
}