#include <elf.h>

#define ELF_MAGIC 0x464C457F // 0x7F, 'E', 'L', 'F'
#define ELF_32_BIT 1
#define ELF_LITTLE_ENDIAN 1
#define ELF_EXECUTABLE 2
#define ELF_X86 3
#define ELF_HEADER_SIZE_32_BIT 52
#define PT_LOAD 1

struct elf32_header
{
	uint32_t eiMagic;
	uint8_t eiClass;
	uint8_t eiData;
	uint8_t eiVersion;
	uint8_t eiOSABI;
	uint8_t eiABIVersion;
	uint32_t pad1;
	uint16_t pad2;
	uint8_t pad3;
	uint16_t eType;
	uint16_t eMachine;
	uint32_t eVersion;
	uint32_t eEntry;
	uint32_t ePhoff;
	uint32_t eShoff;
	uint32_t eFlags;
	uint16_t eEhSize;
	uint16_t ePhEntSize;
	uint16_t ePhNum;
	uint16_t eShEntSize;
	uint16_t eShNum;
	uint16_t eShStrNdx;
} __attribute__((__packed__));

struct elf32_program_header
{
	uint32_t type;
	uint32_t offset;
	uint32_t vAddr;
	uint32_t pAddr;
	uint32_t fileSize;
	uint32_t memSize;
	uint32_t flags;
	uint32_t align;
} __attribute__((__packed__));

inline bool elf_is_valid(void *image)
{
	struct elf32_header *ph = (struct elf32_header *)image;
	
	return (ph->eiMagic == ELF_MAGIC);
}

inline bool elf_is_32_bit(void *image)
{
	struct elf32_header *ph = (struct elf32_header *)image;

	return ((ph->eiClass == ELF_32_BIT) && (ph->eEhSize == ELF_HEADER_SIZE_32_BIT));
}

inline bool elf_is_little_endian(void *image)
{
	struct elf32_header *ph = (struct elf32_header *)image;

	return (ph->eiData == ELF_LITTLE_ENDIAN);
}

inline bool elf_is_executable(void *image)
{
	struct elf32_header *ph = (struct elf32_header *)image;

	return (ph->eType == ELF_EXECUTABLE);
}

inline bool elf_is_x86(void *image)
{
	struct elf32_header *ph = (struct elf32_header *)image;

	return (ph->eMachine == ELF_X86);
}

inline uint32_t elf_entry_point(void *image)
{
	struct elf32_header *ph = (struct elf32_header *)image;

	return ph->eEntry;
}

inline uint32_t elf_pht_offset(void *image)
{
	struct elf32_header *ph = (struct elf32_header *)image;

	return ph->ePhoff;
}

inline uint32_t elf_sht_offset(void *image)
{
	struct elf32_header *ph = (struct elf32_header *)image;

	return ph->eShoff;
}

inline uint16_t elf_phte_size(void *image)
{
	struct elf32_header *ph = (struct elf32_header *)image;

	return ph->ePhEntSize;
}

inline uint16_t elf_pht_entries(void *image)
{
	struct elf32_header *ph = (struct elf32_header *)image;

	return ph->ePhNum;
}

inline uint16_t elf_shte_size(void *image)
{
	struct elf32_header *ph = (struct elf32_header *)image;

	return ph->eShEntSize;
}

inline uint16_t elf_sht_entries(void *image)
{
	struct elf32_header *ph = (struct elf32_header *)image;

	return ph->eShNum;
}

inline uint16_t elf_shte_names_index(void *image)
{
	struct elf32_header *ph = (struct elf32_header *)image;

	return ph->eShStrNdx;
}

inline bool elf_lithiumos_check(void *image)
{
	return (elf_is_valid(image) && elf_is_32_bit(image) && elf_is_x86(image) && elf_is_little_endian(image)
		&& elf_is_executable(image) && (elf_pht_entries(image) <= EXE_MAX_SEGMENTS));
}

uint32_t elf32_parse(void *image, struct elf_load_info *pli)
{
	// Sanity check
	if(!elf_lithiumos_check(image))
		return 1;

	struct elf32_program_header *headers = (struct elf32_program_header *)((uint32_t)image + elf_pht_offset(image));
	struct elf_segment *segments = (struct elf_segment *)(pli->segs);

	pli->numSegs = elf_pht_entries(image);
	pli->entryPoint = elf_entry_point(image);

	for(uint32_t i = 0; i < pli->numSegs; ++i)
	{
		if(headers[i].type == PT_LOAD)
		{
			segments[i].offsetInFile = headers[i].offset;
			segments[i].addressInMemory = headers[i].vAddr;
			segments[i].sizeInFile = headers[i].fileSize;
			segments[i].sizeInMemory = headers[i].memSize;
		}
	}

	return 0;
}
