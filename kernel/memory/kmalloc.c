/*
******** BMW's kernel memory allocator version 0.2 29/11/2013 ********

This kernel memory allocator requires access to the following:

1. _KERNEL_END_ symbol which is located at the end of the kernel.
The address of this symbol (&_KERNEL_END_) is used for the start of the
heap.

2. The following types: uint32_t, size_t, bool, byte, virtual_addr.

3. The following definitions: FALSE = 0, TRUE = 1.

4. A virtual memory manager with a function which does the same as
the vmmngr_alloc_page() function used in this code. The vmmngr_alloc_page()
function must guarantee that the specified page is mapped into physical
memory.

NOTES:
You may want to modify the KHEAP_END, PAGE_SIZE and MIN_BLOCK_SIZE
definitions below to suit your OS. _KERNEL_END_ and KHEAP_END must be
aligned on a PAGE_SIZE boundary.

You must call kmalloc_init() before using the kernel memory allocator.

The status field of the kmallocHeader struct is 32 bits for a reason -
you could use it to store information about the memory request.
*/

#include <kmalloc.h>
#include <vmmngr.h>

#define KHEAP_END 0xF0000000
#define PAGE_SIZE 4096
#define MIN_BLOCK_SIZE 4

extern byte _KERNEL_END_;

static void *KHEAP_START;

struct kmallocHeader
{
	uint32_t status;
	struct kmallocHeader *previousHeader;
	struct kmallocHeader *nextHeader;
} __attribute__((__packed__));

bool kmalloc_init(void)
{
	KHEAP_START = &_KERNEL_END_;

	// Make sure pages for starting headers are mapped
	if(!vmmngr_alloc_page((virtual_addr)KHEAP_START))
		return FALSE;

	if(!vmmngr_alloc_page((virtual_addr)(KHEAP_END - PAGE_SIZE)))
		return FALSE;

	// Set up first header
	struct kmallocHeader *header = (struct kmallocHeader *)KHEAP_START;

	// Set to zero to mark it as free
	header->status = 0;

	// Pointer to previous header - zero means no previous header
	header->previousHeader = (struct kmallocHeader *)0; 

	// Set to end of heap
	header->nextHeader = (struct kmallocHeader *)(KHEAP_END - sizeof(struct kmallocHeader));

	// Set up last header
	header = (struct kmallocHeader *)(KHEAP_END - sizeof(struct kmallocHeader));

	// Set last entry to used - it is 0 bytes in size anyway
	header->status = 0x00000001;

	// Set previous header pointer
	header->previousHeader = (struct kmallocHeader *)(KHEAP_START);

	// Set next header pointer
	header->nextHeader = (struct kmallocHeader *)(KHEAP_END - sizeof(struct kmallocHeader));

	return TRUE;
}

void *kmalloc(size_t bytes)
{
	// A few sanity checks
	if(bytes == 0)
		return NULL;

	if(bytes < MIN_BLOCK_SIZE)
		bytes = MIN_BLOCK_SIZE;

	if(bytes > (uint32_t)(KHEAP_END - (uint32_t)KHEAP_START - (2 * sizeof(struct kmallocHeader))))
		return NULL;

	struct kmallocHeader *currentHeader = (struct kmallocHeader *)KHEAP_START;
	struct kmallocHeader *nextHeader = NULL;
	struct kmallocHeader *newHeader = NULL;
	void *retVal = NULL;

	while((uint32_t)currentHeader < (uint32_t)(KHEAP_END - sizeof(struct kmallocHeader)))
	{
		if(currentHeader->status == 0)
		{
			// This block is free, check if it is big enough
			if(((uint32_t)currentHeader->nextHeader - (uint32_t)currentHeader) >=
				(uint32_t)(sizeof(struct kmallocHeader) + bytes))
			{
				// Block is big enough, allocate space
				// Set header of current block to used status
				currentHeader->status = 0x00000001; // Set used

				// Check if we have to make a new header or one already exists
				if((uint32_t)currentHeader->nextHeader ==
					((uint32_t)currentHeader + sizeof(struct kmallocHeader) + bytes))
				{
					// Header already exists
					nextHeader = currentHeader->nextHeader;

					// Make sure the next header has the correct previous header pointer
					nextHeader->previousHeader = currentHeader;

					// Set pointer to allocated memory
					retVal = (void *)((uint32_t)currentHeader + (uint32_t)sizeof(struct kmallocHeader));

					break;
				}
				else
				{
					// We have to create a new header
					nextHeader = currentHeader->nextHeader;
					newHeader = (struct kmallocHeader*)((uint32_t)currentHeader +
						sizeof(struct kmallocHeader) + bytes);

					// Check if our new header will overlap any other headers or will be too close
					if(((uint32_t)nextHeader - (uint32_t)newHeader) <
						(sizeof(struct kmallocHeader) + MIN_BLOCK_SIZE))
					{
						// It will overlap or is too close, just use the next header and waste a few bytes
						// Make sure the next header has the correct previous header pointer
						nextHeader->previousHeader = currentHeader;

						// Set pointer to allocated memory
						retVal = (void *)((uint32_t)currentHeader + (uint32_t)sizeof(struct kmallocHeader));

						break;
					}
					else
					{
						// It will not overlap any other headers, go ahead and create a new one
						// Set next header of current header
						currentHeader->nextHeader = newHeader;

						// Set status of new header
						newHeader->status = 0;

						// Set previous header of new header
						newHeader->previousHeader = currentHeader;

						// Set next header of new header
						newHeader->nextHeader = nextHeader;

						// Set previous header of next header
						nextHeader->previousHeader = newHeader;

						// Set pointer to allocated memory
						retVal = (void *)((uint32_t)currentHeader + (uint32_t)sizeof(struct kmallocHeader));

						break;
					}
				}
			}
			else
			{
				// Block is not big enough, go to next block
				currentHeader = currentHeader->nextHeader;
			}
		}
		else
		{
			// Block is not free, go to next block
			currentHeader = currentHeader->nextHeader;
		}
	}
	
	// Make sure area is mapped in virtual memory
	uint32_t sizeToCheck = bytes;
	
	for(uint32_t addr = (uint32_t)retVal; (addr & 0xFFFFF000) <= ((uint32_t)retVal + sizeToCheck);
		addr += PAGE_SIZE)
	{
		// Make sure memory is mapped
		if(!vmmngr_alloc_page(addr))
			return 0;
	}
	
	return retVal;
}

void kfree(void *address)
{
	// Initialise header pointers
	struct kmallocHeader* currentHeader = (struct kmallocHeader *)((uint32_t)address -
		sizeof(struct kmallocHeader));
	struct kmallocHeader* previousHeader = currentHeader->previousHeader;
	struct kmallocHeader* nextHeader = currentHeader->nextHeader;

	// Unallocate that block
	currentHeader->status = 0;

	// Check if we can merge it with previous block
	if((uint32_t)previousHeader != 0)
	{
		if(previousHeader->status == 0)
		{
			// Previous block isn't allocated, we can merge
			previousHeader->nextHeader = nextHeader;
			nextHeader->previousHeader = previousHeader;

			// Check if we can merge it with next block
			if(nextHeader->status == 0)
			{
				// Next block isn't allocated, we can merge
				// Update header pointers from previous merge
				currentHeader = nextHeader;
				nextHeader = currentHeader->nextHeader;

				// Merge
				previousHeader->nextHeader = nextHeader;
				nextHeader->previousHeader = previousHeader;
			}

			return;
		}
		else
		{
			// Check if we can merge it with next block
			if(nextHeader->status == 0)
			{
				// Next block isn't allocated, we can merge
				previousHeader = currentHeader;
				currentHeader = nextHeader;
				nextHeader = currentHeader->nextHeader;

				// Merge
				previousHeader->nextHeader = nextHeader;
				nextHeader->previousHeader = previousHeader;
			}

			return;
		}
	}

	// If we get to here, we must be dealing with the first block
	// Only check if we can merge with next block
	if(nextHeader->status == 0)
	{
		// Next block isn't allocated, we can merge
		previousHeader = currentHeader;
		currentHeader = nextHeader;
		nextHeader = currentHeader->nextHeader;

		// Merge
		previousHeader->nextHeader = nextHeader;
		nextHeader->previousHeader = previousHeader;
	}
	
}
