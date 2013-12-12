#include <funccodes.h>
#include <stdio.h>

struct print_info
{
	unsigned char attrib;
	const char *string;
} __attribute__((__packed__));

void printf(const char *string, unsigned char attrib)
{
	struct print_info pi;
	pi.string = string;
	pi.attrib = attrib;
	struct print_info *ppi = &pi;
	__asm__ __volatile__("movl %0, %%eax" : : "g" (FUNC_CODE_PRINT) : "eax");
	__asm__ __volatile__("movl %0, %%ebx" : : "m" (ppi) : "ebx");
	__asm__ __volatile__("int $0x22");
}
