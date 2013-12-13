#ifndef SYSCALLS_H
#define SYSCALLS_H

#include <types.h>

#define SYSCALL_PRINT 			0
#define SYSCALL_VIRTUAL_ALLOC 	1

void sc_print_string(const char *str);
bool sc_virtual_alloc(uint32_t startAddr, size_t numPages);

#endif