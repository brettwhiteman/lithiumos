/*
Lithium OS virtual memory manager.
*/

#include <vmmngr.h>

#define PAGE_TABLES_ADDR 		0xFFC00000
#define PAGE_DIRECTORY_ADDRESS 	0xFFFFF000

physical_addr currentDirectory = 0;

bool vmmngr_init(physical_addr pd_physical)
{
	if(!pd_physical)
		return FALSE;
	
	currentDirectory = pd_physical;
	
	return TRUE;
}

bool vmmngr_commit_page(pt_entry *e)
{
	void *p = pmmngr_alloc_block();
	
	if(!p)
		return FALSE;
	
	pt_entry_set_frame(e, (physical_addr)p);
	pt_entry_add_attrib(e, PTE_PRESENT);
	pt_entry_add_attrib(e, PTE_WRITABLE);
	
	return TRUE;
}

void vmmngr_free_page(virtual_addr addr)
{
	pt_entry *pte;

	pte = (void *)vmmngr_get_ptable_address(addr) + PAGE_TABLE_INDEX(addr);

	pmmngr_free_block((physical_addr)pt_entry_frame(*pte));
	
	pt_entry_del_attrib(pte, PTE_PRESENT);
}

inline pt_entry *vmmngr_ptable_lookup_entry(ptable *p, virtual_addr addr)
{
	if(p)
		return &p->entries[PAGE_TABLE_INDEX(addr)];
	
	return 0;
}

inline pd_entry *vmmngr_pdirectory_lookup_entry(pdirectory *p, virtual_addr addr)
{
	if(p)
		return &p->entries[PAGE_DIRECTORY_INDEX(addr)];
	
	return (pd_entry*)NULL;
}

inline bool vmmngr_switch_pdirectory(physical_addr pd_physical)
{
	if(!pd_physical)
		return FALSE;
	
	currentDirectory = pd_physical;
	pmmngr_set_cr3(currentDirectory);
	
	return TRUE;
}
 
physical_addr vmmngr_get_directory(void)
{
	return currentDirectory;
}

void vmmngr_flush_tlb_entry(virtual_addr addr)
{
	void *a = (void *)addr;

	__asm__ __volatile__("invlpg %0" : : "m" (*a) : "memory");
}

// void vmmngr_flush_tlb_1024(virtual_addr addr)
// {
// 	disable_interrupts();
// 	for(int i = 0; i < 1024; i++)
// 	{
// 		__asm__ __volatile__("invlpg (%0)" : : "r" (addr) : "memory");
// 		addr += 4096;
// 	}
// 	enable_interrupts();
// }

bool vmmngr_map_page(physical_addr phys, virtual_addr virt)
{
	pdirectory *pd = (pdirectory *)PAGE_DIRECTORY_ADDRESS;
	pd_entry *pde = &pd->entries [PAGE_DIRECTORY_INDEX(virt)];
	
	if((!pd) || (!pde))
		return FALSE;
	
	
	if(!pd_entry_is_present(*pde))
	{
		//make ptable
		ptable *newpt = pmmngr_alloc_block();
		if(!newpt)
			return FALSE;
		
		*pde = (pd_entry)0;
		
		pd_entry_add_attrib(pde, PDE_PRESENT);
		pd_entry_add_attrib(pde, PDE_WRITABLE);
		pd_entry_set_frame(pde, (physical_addr)newpt);
		vmmngr_ptable_clear(vmmngr_get_ptable_address(virt));
	}
	
	ptable *pt = vmmngr_get_ptable_address(virt);
	pt_entry *pte = &pt->entries[PAGE_TABLE_INDEX(virt)];
	
	pt_entry_set_frame(pte, phys);
	pt_entry_add_attrib(pte, PTE_PRESENT);

	return TRUE;
}

void vmmngr_ptable_clear(ptable *pt)
{
	memsetd((uint32_t *)pt, 0, sizeof(ptable) / 4);
}

bool vmmngr_alloc_page(virtual_addr virt)
{
	pdirectory *pd = (pdirectory *)PAGE_DIRECTORY_ADDRESS;
	pd_entry *pde = &pd->entries [PAGE_DIRECTORY_INDEX(virt)];
	
	if((!pd) || (!pde))
		return FALSE;
	
	
	if(!pd_entry_is_present(*pde))
	{
		//make ptable
		ptable *newpt = pmmngr_alloc_block();
		
		if(!newpt)
			return FALSE;
		
		*pde = (pd_entry)0;
		
		pd_entry_add_attrib(pde, PDE_PRESENT);
		pd_entry_add_attrib(pde, PDE_WRITABLE);
		pd_entry_set_frame(pde, (physical_addr)newpt);
		vmmngr_ptable_clear(vmmngr_get_ptable_address(virt));
	}
	
	ptable *pt = vmmngr_get_ptable_address(virt);
	
	pt_entry *pte = &pt->entries[PAGE_TABLE_INDEX(virt)];
	
	if(pt_entry_is_present(*pte))
		return TRUE;
	
	if(!vmmngr_commit_page(pte))
		return FALSE;
	
	return TRUE;
}

ptable *vmmngr_get_ptable_address(virtual_addr addr)
{
	return (ptable *)(PAGE_TABLES_ADDR + PAGE_DIRECTORY_INDEX(addr) * PAGE_SIZE);
}
