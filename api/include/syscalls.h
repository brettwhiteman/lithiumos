#ifndef SYSCALLS_H
#define SYSCALLS_H

#include <types.h>

#define SYSCALL_PRINT 			0
#define SYSCALL_VIRTUAL_ALLOC 	1
#define SYSCALL_EXIT			2

void sc_print_string(const char *str);
bool sc_virtual_alloc(uint32_t startAddr, size_t numPages);
void sc_exit(int status);

#endif