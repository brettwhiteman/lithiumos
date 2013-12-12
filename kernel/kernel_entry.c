/*
Lithium OS kernel initialisation

30/11/2013 - BMW:	Added more information printing as OS loads.
*/

#include <interrupt.h>
#include <print.h>
#include <timer.h>
#include <keyboard.h>
#include <pmmngr.h>
#include <vmmngr.h>
#include <stdinc.h>
#include <gdt.h>
#include <syscall.h>
#include <scheduler.h>
#include <kmalloc.h>

#include "lishell.h"

#define PAGEDIR_PHYSICAL_ADDRESS	0x00080000
#define PAGEDIR_VIRTUAL_ADDRESS		0xFFFFF000
#define VIDMEM_PHYSICAL_ADDRESS		0x000B8000
#define VIDMEM_VIRTUAL_ADDRESS		0xFFBFA000
#define MEMBITMAP_PHYSICAL_ADDRESS	0x00100000
#define MEMBITMAP_VIRTUAL_ADDRESS	0xFFBBA000
#define GDT_PHYSICAL_ADDRESS		0x00000000
#define GDT_VIRTUAL_ADDRESS			0xFFBB9000

struct memoryMapEntry
{
	uint32_t baseL; // Base address QWORD
	uint32_t baseH;
	uint32_t lengthL; // Length QWORD
	uint32_t lengthH;
	uint16_t type; // Region type
	uint16_t acpi; // Extended
	uint32_t padding; // Padding to make it 24 bytes
 
} __attribute__((__packed__));

void initialise_memory(void *ptrMemoryMap, uint32_t uiMemoryMapEntryCount);

void kmain(void *ptrMemoryMap, uint32_t memoryMapEntryCount)
{
	// Self-explanatory functions
	set_colour(0x0F);
	clear_screen();
	print_string("Lithium OS - Loading...\n");
	setup_interrupts();
	print_string("IDT installed\n");
	timer_install();
	keyboard_install();

	initialise_memory(ptrMemoryMap, memoryMapEntryCount);

	scheduler_setup_tss();

	// Install the system call interrupt handler
	irq_install_handler(2, call_handler);

	uint32_t id = scheduler_add_process((void *)lishell, sizeof(lishell));

	if(id == 0)
		print_string("Adding process failed!\n");
	else
	{
		char buf[12] = {0};
		itoa(id, buf, 10);

		print_string("Process with ID ");
		print_string(buf);
		print_string(" added\n");

		// id = scheduler_add_process((void *)TEST_BIN, sizeof(TEST_BIN));

		// itoa(id, buf, 10);

		// print_string("Process with ID ");
		// print_string(buf);
		// print_string(" added\n");

		// scheduler_add_thread(id, (void *)0x1000);
		// scheduler_add_thread(id, (void *)0x1000);
		// scheduler_add_thread(id, (void *)0x1000);
		// scheduler_add_thread(id, (void *)0x1000);
		// scheduler_add_thread(id, (void *)0x1000);
	}

	// Interrupts are still disabled from the stage 2 bootloader
	enable_interrupts();

	while(1)
	{
		halt_cpu();
	}
}

void initialise_memory(void *ptrMemoryMap, uint32_t memoryMapEntryCount)
{
	uint32_t memSize = 0;

	struct memoryMapEntry* ptrMemoryMapEntry = (struct memoryMapEntry *)ptrMemoryMap;

	print_string("Parsing BIOS memory map... ");

	// Calculate the usable memory size from the BIOS memory map
	for(uint32_t i = 0; i < memoryMapEntryCount; i++)
	{
		// Any memory above 4GiB is being ignored since this is a 32-bit OS
		if(ptrMemoryMapEntry[i].baseH == 0)
			memSize += ptrMemoryMapEntry[i].lengthL;
	}

	print_string("done\n");
	
	// Initialise the physical memory manager
	pmmngr_init(memSize, (physical_addr)MEMBITMAP_PHYSICAL_ADDRESS);

	print_string("Physical memory manager initialised\n");

	// Process BIOS memory map
	for(uint32_t i = 0; i < memoryMapEntryCount; i++)
	{
		// Check if free for use or ACPI reclaimable
		if(ptrMemoryMapEntry[i].type == 1 || ptrMemoryMapEntry[i].type == 3)
		{
			// Initialise region - makes it free for use
			pmmngr_init_region(ptrMemoryMapEntry[i].baseL, ptrMemoryMapEntry[i].lengthL);
		}
	}

	// Calculate usable memory in KiB
	uint32_t usableMem = pmmngr_get_free_block_count() * PAGE_SIZE / 1024;
	char printBuf[12] = {0};
	itoa(usableMem, printBuf, 10);
	print_string("Initialised ");
	print_string(printBuf);
	print_string(" KiB of usable memory\n");

	// Calculate the location and size of the extended BIOS data area
	uint32_t ebdaBase = (uint32_t)(*(uint16_t*)0x040E) << 4;

	if(ebdaBase > 0xA0000)
		ebdaBase >>= 4;

	uint32_t ebdaLength = 0xA0000 - ebdaBase;
	
	// De-initialise regions that we have used
	pmmngr_deinit_region(0x00, 0x1000); // GDT
	pmmngr_deinit_region(0x7F000, 0x1000); // Kernel stack
	pmmngr_deinit_region(ebdaBase, ebdaLength); // EBDA
	pmmngr_deinit_region(0xA0000, 0x60000); // Video memory/ROM area
	pmmngr_deinit_region(MEMBITMAP_PHYSICAL_ADDRESS, 0x40000); // Physical memory manager bitmap
	pmmngr_deinit_region(PAGEDIR_PHYSICAL_ADDRESS, 0x4000); // Page directory and 3 page tables
	pmmngr_deinit_region(0x01000000, 0x64000); // The kernel (400KiB max)

	// Recursive paging - we can use physical address since first 4MiB is identity mapped
	pd_entry *pde =  vmmngr_pdirectory_lookup_entry((pdirectory *)PAGEDIR_PHYSICAL_ADDRESS, 0xFFFFFFFF);
	pd_entry_set_frame(pde, PAGEDIR_PHYSICAL_ADDRESS);
	pd_entry_add_attrib(pde, PDE_PRESENT);
	
	// Initialise virtual memory manager. If it fails print a message and halt system.
	if(!vmmngr_init((physical_addr)PAGEDIR_PHYSICAL_ADDRESS))
	{
		print_string("vmmngr_init() failed! System halted.");
		disable_interrupts();
		halt_cpu();
	}

	print_string("Virtual memory manager initialised\n");

	// Map video memory
	vmmngr_map_page(VIDMEM_PHYSICAL_ADDRESS, VIDMEM_VIRTUAL_ADDRESS);
	vmmngr_map_page(VIDMEM_PHYSICAL_ADDRESS + 4096, VIDMEM_VIRTUAL_ADDRESS + 4096);
	vmmngr_map_page(VIDMEM_PHYSICAL_ADDRESS + 8192, VIDMEM_VIRTUAL_ADDRESS + 8192);
	vmmngr_map_page(VIDMEM_PHYSICAL_ADDRESS + 12288, VIDMEM_VIRTUAL_ADDRESS + 12288);
	
	// Update the video memory pointer
	set_vid_mem((void *)VIDMEM_VIRTUAL_ADDRESS);
	
	// Map the memory management bitmap. The bitmap is 256KiB in size, so that's (256 / 4 = 64) pages.
	for(uint32_t i = 0; i < 64; i++)
	{
		vmmngr_map_page((physical_addr)(MEMBITMAP_PHYSICAL_ADDRESS + i * 4096),
			(virtual_addr)(MEMBITMAP_VIRTUAL_ADDRESS + i * 4096));
	}
	
	// Update the memory bitmap pointer
	pmmngr_set_bitmap_address((void *)MEMBITMAP_VIRTUAL_ADDRESS);
	
	// Map GDT
	vmmngr_map_page((physical_addr)GDT_PHYSICAL_ADDRESS, (virtual_addr)GDT_VIRTUAL_ADDRESS);

	// Set GDT address
	// TODO: set_gdt_address(GDT_VIRTUAL_ADDRESS);
	// ATM it is assuming the correct address
	
	// Setup GDT
	gdt_set_entry(0, 0x0000000000000000); // NULL descriptor.
	//								  limit		 base	 access  granularity
	gdt_set_entry(1, create_gdt_entry(0xFFFFF, 0x00000000, 0x9A, 0xC)); // Kernel code descriptor
	gdt_set_entry(2, create_gdt_entry(0xFFFFF, 0x00000000, 0x92, 0xC)); // Kernel data descriptor
	gdt_set_entry(3, create_gdt_entry(0xFFFFF, 0x00000000, 0xFA, 0xC)); // User code descriptor
	gdt_set_entry(4, create_gdt_entry(0xFFFFF, 0x00000000, 0xF2, 0xC)); // User data descriptor
	
	// Load GDT with 5 entries
	load_gdt(5, FALSE);

	print_string("GDT updated\n");
	
	// Un-identity-map first 4MiB
	pde =  vmmngr_pdirectory_lookup_entry((pdirectory *)PAGEDIR_VIRTUAL_ADDRESS, 0x00000000);
	pd_entry_del_attrib(pde, PDE_PRESENT);

	// The first page table (used for first 4MiB) was at 0x81000, so we can free that memory now
	pmmngr_init_region((physical_addr)0x81000, 0x1000);
	
	// Initialise kernel memory allocator
	if(!kmalloc_init())
	{
		print_string("\nkmalloc_init() failed! System halted.\n");
		disable_interrupts();
		halt_cpu();
	}

	print_string("Kernel memory allocator initialised\n");
}
