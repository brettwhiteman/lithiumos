#include <stdlib.h>

extern int main();

void __lios_startup(void)
{
	if(!malloc_init())
		exit(EXIT_FAILURE);



	exit(main());
}