/*
Lithium OS GDT related functions.
*/

#include <gdt.h>

#define GDT_ADDRESS 0xFFBB9000

struct gdtEntry
{
	uint16_t limitLow;
	uint16_t baseLow;
	uint8_t baseMid;
	uint8_t access;
	uint8_t granularity;
	uint8_t baseHigh;
} __attribute__((packed));

struct gdtInfo
{
	uint16_t size;
	uint32_t address;
} __attribute__((packed));

uint64_t create_gdt_entry(uint32_t limit, uint32_t base, uint8_t access, uint8_t granularity)
{	
	struct gdtEntry gdte;
	gdte.limitLow = limit & 0xFFFF;
	gdte.baseLow = base & 0xFFFF;
	gdte.baseMid = (base >> 16) & 0xFF;
	gdte.access = access;
	gdte.granularity = ((limit >> 16) & 0xF) | ((granularity << 4) & 0xF0);
	gdte.baseHigh = (base & 0xFF000000) >> 24;
	
	return *(uint64_t*)&gdte;
}

void gdt_set_entry(uint32_t num, uint64_t descriptor)
{
	*(uint64_t *)(GDT_ADDRESS + num * sizeof(uint64_t)) = descriptor;
}

void load_gdt(uint32_t descriptorcount)
{
	struct gdtInfo gdti;
	gdti.size = 8 * descriptorcount - 1;
	gdti.address = GDT_ADDRESS;
	
	struct gdtInfo* ptr = &gdti;
	
	disable_interrupts();
	__asm__ __volatile__("lgdt %0" : : "m" (*ptr));
	enable_interrupts();
}