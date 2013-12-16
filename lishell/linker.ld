ENTRY(__lios_startup)
OUTPUT_FORMAT("elf32-i386")

SECTIONS
{
    . = 0x1000;

    .text :
    {
        *(.text)
    }

    .rodata :
    {
        *(.rodata)
    }

    .data :
    {
        *(.data)
    }

    .bss :
    {
        *(.bss)
    }

    /* For malloc() */
    __end = .;
}
