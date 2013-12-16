#include <stdlib.h>
#include <syscalls.h>

void exit(int status)
{
	sc_exit(status);
}