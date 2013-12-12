#include <syscall.h>
#include <print.h>
#include <stdinc.h>

#define FUNC_CODE_PRINT 		0x00000000

void call_handler(isr_t *stk)
{
	switch(stk->eax)
	{
	case FUNC_CODE_PRINT:
		print_string((const char *)stk->ebx);
		break;
	}
}
