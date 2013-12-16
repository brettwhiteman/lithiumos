#include <syscalls.h>

void sc_print_string(const char *str)
{
	__asm__ volatile ("movl %0, %%ebx\n"
				  "movl %1, %%eax\n"
				  "int $0x22"
				  :
				  : "r" (str), "i" (SYSCALL_PRINT)
				  : "ebx", "eax");
}

bool sc_virtual_alloc(uint32_t startAddr, size_t numPages)
{
	uint32_t res = FALSE;

	__asm__ volatile ("movl %1, %%ebx\n"
					  "movl %2, %%ecx\n"
					  "movl %3, %%eax\n"
					  "int $0x22\n"
					  "movl %%eax, %0"
					  : "=m" (res)
					  : "m" (startAddr), "m" (numPages), "i" (SYSCALL_VIRTUAL_ALLOC)
					  : "eax", "ebx", "ecx");

	return (bool)res;
}

void sc_exit(int status)
{
	__asm__ volatile("movl %0, %%eax\n"
					 "movl %1, %%ebx\n"
					 "int $0x22"
					 :
					 : "i" (SYSCALL_EXIT), "m" (status)
					 : "eax", "ebx");
}
