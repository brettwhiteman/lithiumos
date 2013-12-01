#ifndef PMMNGR_H
#define PMMNGR_H

#include "stdinc.h"

typedef uint32_t physical_addr;

void pmmngr_init (uint32_t memSize, physical_addr bitmap);
void pmmngr_set_bitmap_address(void *addr);
void mmap_set (uint32_t bit);
void mmap_unset (uint32_t bit);
bool mmap_test (uint32_t bit);
uint32_t mmap_first_free(void);
uint32_t pmmngr_get_block_count(void);
uint32_t pmmngr_get_free_block_count(void);
void pmmngr_init_region(physical_addr base, uint32_t size);
void pmmngr_deinit_region(physical_addr base, uint32_t size);
void *pmmngr_alloc_block(void);
void pmmngr_free_block (physical_addr p);
void pmmngr_set_cr3(physical_addr pdb);
void pmmngr_paging_enable(bool enable);
void *pmmngr_alloc_blocks(uint32_t amount);
void pmmngr_free_blocks (void *p, uint32_t amount);
uint32_t mmap_first_free_s (uint32_t size);

#endif
