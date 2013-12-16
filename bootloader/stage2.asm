;*******************************************************
; Lithium OS Stage 2 Bootloader
;
; Gets a BIOS memory map, enters protected mode,
; sets paging up then jumps to the kernel entry point.

bits 16
org 0x0700

jmp 0x0000:stage2_start

%define KernelAddress 0x8200
%define KernelVirtualAddress 0xC0000000
%define NewKernelAddress 0x01000000
%define MemMapAddress 0x00000B00
%define PageDirAddress 0x80000
%define PageTableFirst4MiB 0x81000
%define PageTableKernel 0x82000
%define PageTableKernelStack 0x83000
%define KernelStackPhysical 0x00080000
%define KernelStackVirtual 0xF0002000

mmapEntries dw 0x0000
memSize dd 0x00000000
e820Error db 'BIOS memory map failed', 0x00

;GDT--------------------------------------
BGDT:
	; Empty descriptor
	dd 0x00000000
	dd 0x00000000

	; Kernel code descriptor
	dw 0xFFFF ;seg length low word
	dw 0x0000 ;base address low word
	db 0x00 ;base address mid byte
	db 10011010b ;type etc
	db 11001111b ;flags and length high nibble
	db 0x00 ;base address high byte
	
	; Kernel data descriptor
	dw 0xFFFF ;seg length low word
	dw 0x0000 ;base address low word
	db 0x00 ;base address mid byte
	db 10010010b ;type etc
	db 11001111b ;flags and length high nibble
	db 0x00 ;base address high byte
EGDT:

GDTInfo:
	dw EGDT - BGDT - 1 ;limit (size)
	dd BGDT
	
stage2_start:
	; Get a BIOS memory map
	xor ax, ax
	mov es, ax
	mov edi, MemMapAddress
	call do_e820
	jc e820_err
	;map entry count is in bp
	mov word [mmapEntries], bp
	
	; Install GDT and enter protected mode
	lgdt [GDTInfo]
	cli
	mov eax, cr0
	or al, 00000001b
	mov cr0, eax
	; Now in ***PROTECTED  MODE*** :D
	jmp 0x08:protected_mode_start

e820_err:
	mov esi, e820Error
	call print
	cli
	hlt

;********************************
; Prints string
; INPUTS:
; ESI = pointer to string
;
; TRASHES: EAX

print:
	mov ah, 0x0E
	lodsb
	cmp al, byte 0x00
	je print_finished
	int 0x10
	jmp print

print_finished:
	ret


;**************************************************************
; BIOS MEMORY MAP (INT 0x15 EAX=0xE820)
; Use the INT 0x15, EAX=0xE820 BIOS function to get a memory map
; INPUTS:
; ES:DI -> destination buffer for 24 byte entries
;
; OUTPUTS:
; BP = entry count
;
; TRASHES: All registers except ESI

do_e820:
	xor ebx, ebx ; EBX must be 0 to start
	xor bp, bp ; Keep an entry count in bp
	mov edx, dword 0x534D4150 ; Place "SMAP" into edx
	mov eax, 0xE820
	mov dword [es:di + 20], 1 ; Force a valid ACPI 3.X entry
	mov ecx, 24 ; Ask for 24 bytes
	int 0x15
	jc short .failed ; Carry set on first call means "unsupported function"
	mov edx, dword 0x0534D4150 ; Some BIOSes apparently trash this register
	cmp eax, edx ; On success, eax must have been reset to "SMAP"
	jne short .failed
	test ebx, ebx ; EBX=0 implies list is only 1 entry long (worthless)
	je short .failed
	jmp short .jmpin
.e820lp:
	mov eax, 0xE820 ; EAX, ECX get trashed on every int 0x15 call
	mov [es:di + 20], dword 1 ; Force a valid ACPI 3.X entry
	mov ecx, 24 ; Ask for 24 bytes again
	int 0x15
	jc short .e820f ; Carry set means "end of list already reached"
	mov edx, dword 0x0534D4150 ; Repair potentially trashed register
.jmpin:
	jcxz .skipent ; Skip any 0 length entries
	cmp cl, 20 ; Got a 24 byte ACPI 3.X response?
	jbe short .notext
	test byte [es:di + 20], 1 ; If so, is the "ignore this data" bit clear?
	je short .skipent
.notext:
	mov ecx, [es:di + 8] ; Get lower dword of memory region length
	or ecx, [es:di + 12] ; "or" it with upper dword to test for zero
	jz .skipent ; If length qword is 0, skip entry
	inc bp ; Got a good entry: ++count, move to next storage spot
	add di, 24
.skipent:
	test ebx, ebx ; If ebx is 0, list is complete
	jne short .e820lp
.e820f:
	clc ; Clear carry flag since there is no error
	ret
.failed:
	stc ; Set carry flag to indicate error
	ret



;************** Protected Mode *******************
bits 32

protected_mode_start:
	; Set up segment registers
	mov ax, 0x0010 ; Kernel data descriptor
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax

	; Set stack up - we use the kernel stack for the
	; stage 2 bootloader since we need to push
	; arguments onto the stack
	mov esp, KernelStackPhysical
	
	; Enable A20 line
	in al, 0x92
	or al, 2
	out 0x92, al
	
	; Copy kernel to its new location
	mov esi, KernelAddress
	mov edi, NewKernelAddress
	mov ecx, 0x00019000 ; Size of kernel (400KiB max) / 4
	cld
	rep movsd
	
	; Set paging up :D
	call setup_paging
	
	; Set the correct stack pointer now that we
	; are using virtual addresses due to paging
	mov esp, KernelStackVirtual
	
	; Execute the Lithium OS Kernel Initialisation
	; Push args onto stack in reverse order then jump
	movzx edx, word [mmapEntries]
	push edx
	mov edx, MemMapAddress
	push edx
	mov ebx, KernelVirtualAddress
	; Prepare for epicness of Lithium OS Kernel!!!
	call ebx

;*******************************
; Sets paging up
; No inputs or outputs
;
; TRASHES: EAX, EBX, ECX, EDI

setup_paging:
	; Clear out paging structures for use
	mov edi, PageDirAddress
	call clear_page_table ; It's not a page table but same size so who cares

	mov edi, PageTableFirst4MiB
	call clear_page_table

	mov edi, PageTableKernel
	call clear_page_table

	mov edi, PageTableKernelStack
	call clear_page_table
	
	; Install the page tables into the page directory
	; Note: all the "+ 1"s are for setting the present bit

	; Page table for first 4 MiB
	mov ebx, PageDirAddress
	mov dword [ebx], PageTableFirst4MiB + 1

	; Page table for kernel
	mov eax, KernelVirtualAddress
	call page_directory_offset
	lea ebx, [PageDirAddress + eax]
	mov dword [ebx], PageTableKernel + 1

	; Page table for kernel stack
	mov eax, KernelStackVirtual
	call page_directory_offset
	lea ebx, [PageDirAddress + eax]
	mov dword [ebx], PageTableKernelStack + 1
	
	; Identity map first 4 MiB
	mov ebx, PageTableFirst4MiB
	mov eax, 1 ; Start at 0, +1 for present bit
.loop_first_4mib:
	mov dword [ebx], eax
	add ebx, 4
	add eax, 4096 ; 4096 = page size
	cmp ebx, PageTableFirst4MiB + 4096 ; Check if we have reached end of page table
	jb .loop_first_4mib
	
	; Map kernel to 0xC0000000 (kernel at 3 GiB virtual)
	mov ebx, PageTableKernel
	mov eax, NewKernelAddress + 1 ; Start at NewKernelAddress, +1 for present bit
.loop_kernel:
	mov dword [ebx], eax
	add ebx, 4
	add eax, 4096 ; 4096 = page size
	cmp ebx, PageTableKernel + 4096 ; Check if we have reached end of page table
	jb .loop_kernel
	
	; Map kernel stack
	mov eax, KernelStackVirtual - 0x2000 ; Map pages before stack address as stack grows down
	call page_directory_offset
	lea ebx, [PageDirAddress + eax]
	mov dword [ebx], PageTableKernelStack + 1
	mov eax, KernelStackVirtual - 0x2000
	call page_table_offset
	lea ebx, [PageTableKernelStack + eax]
	mov dword [ebx], KernelStackPhysical - 0x2000 + 1
	; Repeat for next page
	mov eax, KernelStackVirtual - 0x1000
	call page_table_offset
	lea ebx, [PageTableKernelStack + eax]
	mov dword [ebx], KernelStackPhysical - 0x1000 + 1
	
	; Enable paging
	mov eax, PageDirAddress
	mov cr3, eax
	mov eax, cr0
	or eax, 0x80000000 ; Set paging bit in CR3
	mov cr0, eax ; Paging is officially enabled! :D
	ret

;**************************
; Clears a page table
; INPUT:
; EDI = Address of page table
;
; TRASHES: EAX, ECX

clear_page_table:
	xor ecx, ecx
	xor eax, eax
	
.loop_pt_clear:
	mov dword [edi], eax
	inc ecx
	add edi, 4
	cmp ecx, 1024 ; 4096 (page table size) / 4 (DWORD size)
	jb .loop_pt_clear
	ret

;**************************
; Converts virtual address into page directory offset
; INPUT:
; EAX = Virtual address
;
; OUTPUT:
; EAX = Page directory offset
;
; TRASHES: ECX

page_directory_offset:
	shr eax, 22
	and eax, 0x3FF ; Get lowest 10 bits
	mov ecx, 4
	mul ecx ; Entries are 4 bytes in size
	ret

;**************************
; Converts a virtual address into a page table offset
; INPUT:
; EAX = Virtual address
;
; OUTPUT:
; EAX = Page table offset
;
; TRASHES: ECX

page_table_offset:
	shr eax, 12
	and eax, 0x3FF ; Get lowest 10 bits
	mov ecx, 4
	mul ecx ; Entries are 4 bytes in size
	ret
