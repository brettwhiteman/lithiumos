/*
Lithium OS page directory entry manipulation functions.
*/

#include <pde.h>

inline void pd_entry_add_attrib(pd_entry* e, uint32_t attrib)
{
	*e |= attrib;
}

inline void pd_entry_del_attrib(pd_entry* e, uint32_t attrib)
{
	*e &= ~attrib;
}

inline void pd_entry_set_frame(pd_entry* e, physical_addr addr)
{
	*e = (*e & ~PDE_FRAME) | addr;
}

inline bool pd_entry_is_present(pd_entry e)
{
	return (e & PDE_PRESENT);
}

inline bool pd_entry_is_user(pd_entry e)
{
	return (e & PDE_USER);
}

inline bool pd_entry_is_4mb(pd_entry e)
{
	return (e & PDE_4MB);
}

inline bool pd_entry_is_writable(pd_entry e)
{
	return (e & PDE_WRITABLE);
}

inline physical_addr pd_entry_frame(pd_entry e)
{
	return (e & PDE_FRAME);
}
