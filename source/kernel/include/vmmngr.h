#ifndef VMMNGR_H
#define VMMNGR_H

#include <stdinc.h>
#include <pde.h>
#include <pte.h>

#define PAGES_PER_TABLE 	1024
#define PAGES_PER_DIR		1024
#define PAGE_SIZE 			4096
#define PAGE_DIRECTORY_SIZE	4096

#define PAGE_DIRECTORY_INDEX(x) (((x) >> 22) & 0x3ff)
#define PAGE_TABLE_INDEX(x) (((x) >> 12) & 0x3ff)

typedef uint32_t virtual_addr;

typedef struct
{
	pt_entry entries[PAGES_PER_TABLE];
} ptable;

typedef struct
{
	pd_entry entries[PAGES_PER_DIR];
} pdirectory;

bool vmmngr_init(physical_addr pd_physical);
bool vmmngr_commit_page(pt_entry *e);
void vmmngr_free_page(virtual_addr addr);
pt_entry* vmmngr_ptable_lookup_entry(ptable *p, virtual_addr addr);
pd_entry* vmmngr_pdirectory_lookup_entry(pdirectory *p, virtual_addr addr);
bool vmmngr_switch_pdirectory(physical_addr pd_physical);
physical_addr vmmngr_get_directory(void);
void vmmngr_flush_tlb_entry(virtual_addr addr);
void vmmngr_flush_tlb_1024(virtual_addr addr);
bool vmmngr_map_page (physical_addr phys, virtual_addr virt);
void vmmngr_ptable_clear(ptable *pt);
bool vmmngr_alloc_page(virtual_addr virt);
ptable* get_ptable_address(virtual_addr addr);

#endif
