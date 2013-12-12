#include <stdio.h>

void main(void)
{
	printf("Hello World!\n", 0x30);

	while(1); // Stop it from returning to nowhere
}
