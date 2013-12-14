#include <stdio.h>
#include <stdlib.h>

void main(void)
{
	if(malloc_init())
	{
		printf("malloc_init() succeeded\n");

		const size_t allocSize = 8192;

		// Test Malloc
		void *alloc = malloc(allocSize);

		printf("malloc(%d) returned %x\nTesting writing to address %x... ", allocSize, alloc, alloc);

		for(uint32_t i = 0; i < allocSize; ++i)
		{
			*(uint8_t *)((uint32_t)alloc + i) = 0xEF;
		}

		printf("done\n");

		void *alloc2 = malloc(3);

		printf("malloc(3) returned %x, testing write... ", alloc2);

		*(char *)alloc2 = 'f';
		*(char *)((uint32_t)alloc2 + 1) = 'g';
		*(char *)((uint32_t)alloc2 + 2) = 'g';

		printf("done\nFreeing memory allocated at %x\n", alloc);

		free(alloc);

		alloc = malloc(16);

		printf("malloc(16) returned %x\n", alloc);

		void *alloc3 = malloc(32);

		printf("malloc(32) returned %x\n", alloc3);

	}
	else
		printf("malloc_init() failed\n");

	while(1); // Stop it from returning to nowhere
}
