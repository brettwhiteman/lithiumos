/*
Lithium OS page table entry manipulation functions.
*/

#include <pte.h>

inline void pt_entry_add_attrib(pt_entry *e, uint32_t attrib)
{
	*e |= attrib;
}

inline void pt_entry_del_attrib(pt_entry *e, uint32_t attrib)
{
	*e &= ~attrib;
}

inline void pt_entry_set_frame(pt_entry *e, physical_addr addr)
{
	*e = (*e & ~PTE_FRAME) | addr;
}

inline bool pt_entry_is_present(pt_entry e)
{
	return (e & PTE_PRESENT);
}

inline bool pt_entry_is_writable(pt_entry e)
{
	return (e & PTE_WRITABLE);
}

inline uint32_t pt_entry_frame(pt_entry e)
{
	return (e & PTE_FRAME);
}
