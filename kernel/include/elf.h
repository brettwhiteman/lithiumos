#ifndef ELF_H
#define ELF_H

#include <stdinc.h>

struct elf_segment
{
	uint32_t offsetInFile;
	uint32_t sizeInFile;
	uint32_t addressInMemory;
	uint32_t sizeInMemory;
};

#define EXE_MAX_SEGMENTS 5

struct elf_load_info
{
	struct elf_segment segs[EXE_MAX_SEGMENTS];
	uint32_t numSegs;
	uint32_t entryPoint;
};

bool elf_is_valid(void *image);
bool elf_is_32_bit(void *image);
bool elf_is_little_endian(void *image);
bool elf_is_executable(void *image);
bool elf_is_x86(void *image);
uint32_t elf_entry_point(void *image);
uint32_t elf_pht_offset(void *image);
uint32_t elf_sht_offset(void *image);
uint16_t elf_phte_size(void *image);
uint16_t elf_pht_entries(void *image);
uint16_t elf_shte_size(void *image);
uint16_t elf_sht_entries(void *image);
uint16_t elf_shte_names_index(void *image);
bool elf_lithiumos_check(void *image);
uint32_t elf32_parse(void *image, struct elf_load_info *pli);

#endif