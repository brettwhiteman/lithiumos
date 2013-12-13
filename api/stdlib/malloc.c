/*
Lithium OS LIBC malloc()
*/

#include <malloc.h>
#include <syscalls.h>

#define HEAP_END 0xA0000000
#define PAGE_SIZE 4096
#define MIN_BLOCK_SIZE 4
#define PAGE_SIZE 4096
#define DEFAULT_HEAP_SIZE PAGE_SIZE
#define MEM_STATUS_FREE 0
#define MEM_STATUS_USED 1

#define ROUND_UP_PAGE(x) if(x & (PAGE_SIZE - 1)) { x += PAGE_SIZE; x &= ~(PAGE_SIZE - 1); }
#define ALIGN_DWORD(x) if(x & 3) { x += 4; x &= ~3; }

extern uint32_t __end;

static void *heapStart;
static void *heapEnd;

typedef struct mheader_struct
{
	uint32_t status;
	struct mheader_struct *prev;
	struct mheader_struct *next;
} __attribute__((__packed__)) mheader_t;

bool malloc_init(void)
{
	heapStart = &__end;
	heapEnd = (void *)((uint32_t)heapStart + DEFAULT_HEAP_SIZE);

	// Map starting heap
	if(!sc_virtual_alloc((uint32_t)heapStart, DEFAULT_HEAP_SIZE / PAGE_SIZE))
		return FALSE;

	// Set up first header
	mheader_t *header = (mheader_t *)heapStart;

	header->status = MEM_STATUS_FREE;

	// NULL means no previous header
	header->prev = NULL; 

	// And NULL also means no next header
	header->next = NULL;

	return TRUE;
}

void *malloc(size_t bytes)
{
	// A few sanity checks
	if(bytes == 0)
		return NULL;

	if(bytes < MIN_BLOCK_SIZE)
		bytes = MIN_BLOCK_SIZE;

	// Make sure it is 32-bit aligned
	ALIGN_DWORD(bytes);

	mheader_t *current = (mheader_t *)heapStart;
	mheader_t *next = NULL;
	mheader_t *new = NULL;
	mheader_t *last = NULL;
	size_t freeSpaceAtEnd = 0;

	do
	{
		if(current->status == MEM_STATUS_FREE)
		{
			// This block is free, check if it is big enough
			size_t slotSize = 0;

			if(current->next == NULL) //Last header
				slotSize = (uint32_t)heapEnd - (uint32_t)current - sizeof(mheader_t);
			else // Not last header
				slotSize = (uint32_t)current->next - (uint32_t)current - sizeof(mheader_t);

			if(slotSize >= bytes)
			{
				// Block is big enough, allocate space
				// Set header of current block to used status
				current->status = MEM_STATUS_USED; // Set used

				// Check if we have to make a new header or one already exists
				if(current->next == NULL)
				{
					// Last header

					new = (mheader_t *)((uint32_t)current + sizeof(mheader_t) + bytes);

					// Check if the new header is within heap and leaves enough room
					if((uint32_t)new > ((uint32_t)heapEnd - sizeof(mheader_t) - MIN_BLOCK_SIZE))
					{
						// No room for new header

						return (void *)((uint32_t)current + sizeof(mheader_t));
					}
					else
					{
						// New header fits

						current->next = new;
						new->prev = current;
						new->next = NULL;
						new->status = MEM_STATUS_FREE;

						return (void *)((uint32_t)current + sizeof(mheader_t));
					}
				}
				else if((uint32_t)current->next == ((uint32_t)current + sizeof(mheader_t) + bytes))
				{
					// Header already exists
					next = current->next;

					// Make sure the next header has the correct previous header pointer
					next->prev = current;

					// Set pointer to allocated memory
					return (void *)((uint32_t)current + sizeof(mheader_t));
				}
				else
				{
					// We have to create a new header
					next = current->next;
					new = (mheader_t *)((uint32_t)current + sizeof(mheader_t) + bytes);

					// Check if our new header will overlap any other headers or will be too close
					if(((uint32_t)next - (uint32_t)new) < (sizeof(mheader_t) + MIN_BLOCK_SIZE))
					{
						// It will overlap or is too close, just use the next header and waste a few bytes
						// Make sure the next header has the correct previous header pointer
						next->prev = current;

						// Set pointer to allocated memory
						return (void *)((uint32_t)current + sizeof(mheader_t));
					}
					else
					{
						// It will not overlap any other headers, go ahead and create a new one
						// Set next header of current header
						current->next = new;

						// Set status of new header
						new->status = MEM_STATUS_FREE;

						// Set previous header of new header
						new->prev = current;

						// Set next header of new header
						new->next = next;

						// Set previous header of next header
						next->prev = new;

						// Set pointer to allocated memory
						return (void *)((uint32_t)current + sizeof(mheader_t));
					}
				}
			}
			else
			{
				// Block is not big enough

				if(current->next == NULL)
				{
					freeSpaceAtEnd = slotSize;

					last = current;
				}

				current = current->next;
			}
		}
		else
		{
			// Block is not free, go to next block

			if(current->next == NULL)
			{
				freeSpaceAtEnd = (uint32_t)heapEnd - (uint32_t)current - sizeof(mheader_t);

				last = current;
			}

			current = current->next;
		}
	} while(current != NULL);
	
	
	// Didn't find a suitable memory slot
	// Check if we will have enough memory to expand heap and allocate
	if((HEAP_END - ((uint8_t)heapEnd - freeSpaceAtEnd) - sizeof(mheader_t)) >= bytes)
	{
		size_t expand = bytes + sizeof(mheader_t);

		// Round size up to nearest page
		ROUND_UP_PAGE(expand);

		if(sc_virtual_alloc((uint32_t)heapEnd, expand / PAGE_SIZE))
		{
			heapEnd = (void *)((uint32_t)heapEnd + expand);

			if(last->status == MEM_STATUS_FREE)
			{
				last->status = MEM_STATUS_USED;

				new = (mheader_t *)((uint32_t)last + sizeof(mheader_t) + bytes);

				// Check if the new header is within heap and leaves enough room
				if((uint32_t)new > (heapEnd - sizeof(mheader_t) - MIN_BLOCK_SIZE))
				{
					// No room for new header

					return (void *)((uint32_t)last + sizeof(mheader_t));
				}
				else
				{
					// New header fits

					last->next = new;
					new->prev = last;
					new->next = NULL;
					new->status = MEM_STATUS_FREE;

					return (void *)((uint32_t)last + sizeof(mheader_t));
				}
			}
			else
			{
				// Have to make new header
				new = (mheader_t *)((uint32_t)heapEnd - expand - freeSpaceAtEnd);

				last->next = new;
				new->prev = last;
				new->next = NULL;
				new->status = MEM_STATUS_FREE;

				return (void *)((uint32_t)last + sizeof(mheader_t));
			}
		}
		else
			return NULL; // Out of memory or some other error
	}
	else
		return NULL; // Out of memory
}

void free(void *mem)
{
	// Initialise header pointers
	mheader_t *current = (mheader_t *)((uint32_t)mem - sizeof(mheader_t));
	mheader_t *prev = current->prev;
	mheader_t *next = current->next;

	// Unallocate that block
	current->status = MEM_STATUS_FREE;

	// Check if we can merge it with previous block
	if(prev == NULL)
	{
		// First header, only merge next

		if(next == NULL)
			return;

		if(next->status == MEM_STATUS_FREE)
		{
			// Next block isn't allocated, we can merge
			current->next = next->next;
			
			if(next->next == NULL)
				return;

			next->next->prev = current;
		}
	}
	else
	{
		if(prev->status == MEM_STATUS_FREE)
		{
			// Previous block isn't allocated, we can merge
			prev->next = next;

			if(next == NULL)
				return;

			next->prev = prev;

			// Check if we can merge it with next block
			
			if(next->status == MEM_STATUS_FREE)
			{
				// Next block isn't allocated, we can merge

				prev->next = next->next;

				if(next->next == NULL)
					return;

				next->next->prev = prev;
			}
		}
		else
		{
			if(next == NULL)
				return;

			// Check if we can merge it with next block
			if(next->status == MEM_STATUS_FREE)
			{
				// Next block isn't allocated, we can merge
				current->next = next->next;

				if(next->next == NULL)
					return;

				next->next->prev = current;
			}
		}
	}
}
