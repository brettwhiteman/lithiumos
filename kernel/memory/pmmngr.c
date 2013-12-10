/*
Lithium OS physical memory manager.
*/

#include <pmmngr.h>

#define PMMNGR_BLOCK_SIZE 4096

static uint32_t mmngrMemorySize = 0;
static uint32_t mmngrUsedBlocks = 0;
static uint32_t mmngrMaxBlocks = 0;
static uint32_t* mmngrMemoryMap = 0;

void pmmngr_init(uint32_t memSize, physical_addr bitmapBase)
{
	mmngrMemorySize = memSize;
	mmngrMemoryMap = (uint32_t*)bitmapBase;
	mmngrMaxBlocks = memSize / PMMNGR_BLOCK_SIZE;
	mmngrUsedBlocks = mmngrMaxBlocks;

	//For safety we assume all memory is used.
	memsetd((uint32_t*)mmngrMemoryMap, 0xFFFFFFFF, mmngrMaxBlocks / 32);
}

void pmmngr_set_bitmap_address(void* addr)
{
	mmngrMemoryMap = (uint32_t*)addr;
}

inline void mmap_set(uint32_t bit)
{
	mmngrMemoryMap[bit / 32] |= (1 << (bit % 32));
}

inline void mmap_unset(uint32_t bit)
{
	mmngrMemoryMap[bit / 32] &= ~ (1 << (bit % 32));
}

inline bool mmap_test(uint32_t bit)
{
	if(bit > mmngrMaxBlocks)
		return 1;
	return mmngrMemoryMap[bit / 32] &  (1 << (bit % 32));
}

uint32_t mmap_first_free(void)
{
	//! find the first free bit
	for (uint32_t i = 0; i < pmmngr_get_block_count() / 32; i++)
	{
		if (mmngrMemoryMap[i] != 0xFFFFFFFF)
		{
			for (uint32_t j = 0; j < 32; j++)
			{
				uint32_t bit = 1 << j;
				if (!(mmngrMemoryMap[i] & bit))
					return i * 32 + j;
			}
		}
	}

	return 0;
}

inline uint32_t pmmngr_get_block_count(void)
{
	return mmngrMaxBlocks;
}

inline uint32_t pmmngr_get_free_block_count(void)
{
	return mmngrMaxBlocks - mmngrUsedBlocks;
}

void pmmngr_init_region(physical_addr base, uint32_t size)
{
	//round base up to 4k align
	if(base & 0xFFF)
	{
		//not 4k aligned
		base += 4096;
		base &= ~0xFFF;
	}
	
	uint32_t start = (uint32_t)base / PMMNGR_BLOCK_SIZE;
	
	// round size up to 4k align
	if(size & 0xFFF)
	{
		//not 4k aligned
		size += 4096;
		size &= ~0xFFF;
	}
	
	uint32_t blocks = size / PMMNGR_BLOCK_SIZE;

	for (; blocks > 0; blocks--)
	{
		mmap_unset (start++);
		mmngrUsedBlocks--;
	}

	mmap_set(0);	//first block is always set. This insures allocs cant be 0, 
	//which is returned if the system is out of memory
}

void pmmngr_deinit_region(physical_addr base, uint32_t size)
{
	uint32_t start = base / PMMNGR_BLOCK_SIZE;
	
	// round size up to 4k align
	if(size & 0xFFF)
	{
		//not 4k aligned, align it
		size += 4096;
		size &= ~0xFFF;
	}
	
	uint32_t blocks = size / PMMNGR_BLOCK_SIZE;
 
	for (;blocks > 0; blocks--)
	{
		mmap_set(start++);
		mmngrUsedBlocks++;
	}
}

void *pmmngr_alloc_block(void)
{
	if (pmmngr_get_free_block_count() <= 0)
		return (void*)0;	//out of memory
 
	uint32_t block = mmap_first_free();
 
	if (block == 0)
		return (void*)0;	//out of memory
 
	mmap_set(block);
 
	void* addr = (void*)(block * PMMNGR_BLOCK_SIZE);
	mmngrUsedBlocks++;
 
	return addr;
}

void pmmngr_free_block(physical_addr p)
{
	uint32_t block = (uint32_t)p / PMMNGR_BLOCK_SIZE;
 
	mmap_unset(block);
 
	mmngrUsedBlocks--;
}

void pmmngr_set_cr3(physical_addr pdb)
{
	__asm__ __volatile__("movl %%eax, %%cr3" : : "a" (pdb));
}

void pmmngr_paging_enable(bool enable)
{
	if(enable)
	{
		__asm__ __volatile__("movl %cr0, %eax");
		__asm__ __volatile__("orl $0x80000000, %eax");
		__asm__ __volatile__("movl %eax, %cr0");
	}
	else
	{
		__asm__ __volatile__("movl %cr0, %eax");
		__asm__ __volatile__("andl $0x7FFFFFFF, %eax");
		__asm__ __volatile__("movl %eax, %cr0");
	}
}

void* pmmngr_alloc_blocks(uint32_t amount)
{
	if((pmmngr_get_free_block_count() <= 0) || (amount > pmmngr_get_free_block_count()))
		return (void*)0; //out of memory
	
	uint32_t block = mmap_first_free_s(amount);
	
	if(block == 0)
		return (void*)0; //out of memory
	
	for (uint32_t i=0; i<amount; i++)
		mmap_set(block+i);

	void* addr = (void*)((uint32_t)block * PMMNGR_BLOCK_SIZE);
	
	mmngrUsedBlocks += amount;

	return addr;
}

void pmmngr_free_blocks(void* p, uint32_t amount)
{
	physical_addr addr = (physical_addr)p;
	uint32_t block = addr / PMMNGR_BLOCK_SIZE;

	for (uint32_t i = 0; i < amount; i++)
		mmap_unset (block+i);

	mmngrUsedBlocks -= amount;
}

uint32_t mmap_first_free_s(uint32_t amount)
{
	uint32_t free = 0;
	uint32_t base = 0;
	
	for (uint32_t i = 0; i < (mmngrMaxBlocks / 32); i++)
	{
		if(mmngrMemoryMap[i] == 0xFFFFFFFF)
			continue;
		
		for(uint32_t k = 0; k < 32; k++)
		{
			free = 0;
			base = i * 32 + k;
			if(!mmap_test(i * 32 + k))
			{
				for(uint32_t j = 0; j < amount; j++)
				{
					if(!mmap_test(base + j))
					{
						free++;
					}
					else
					{
						if(free == amount)
							return base;
						
						break;
					}
					
					if(free == amount)
						return base;
					
					k++;
				}
				
				if(k > 31)
				{
					i += k / 32;
				}
			}
		}
	}
	
	return 0;
}
