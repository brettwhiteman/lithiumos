#ifndef GDT_H
#define GDT_H

#include <stdinc.h>

uint64_t create_gdt_entry(uint32_t limit, uint32_t base, uint8_t access, uint8_t granularity);
void gdt_set_entry(uint32_t num, uint64_t descriptor);
void load_gdt(uint32_t descriptorcount);

#endif
