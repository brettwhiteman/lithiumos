#include <syscall.h>
#include <print.h>
#include <stdinc.h>

#define FUNC_CODE_PRINT 		0x00000000

struct printInfo
{
	byte attrib;
	char *string;
} __attribute__((packed));

void call_handler(isr_t *stk)
{
	switch(stk->eax)
	{
	case FUNC_CODE_PRINT:;
		byte oc = get_colour();
		set_colour(((struct printInfo *)(stk->ebx))->attrib);
		print_string(((struct printInfo *)(stk->ebx))->string);
		set_colour(oc);
		break;
	}
}
