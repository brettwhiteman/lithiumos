bits 16
org 0x0700

jmp 0x0000:stage2_start

%define KernelAddress 0x8200
%define KernelVirtAddress 0xC0000000
%define NewKernelAddress 0x01000000
%define MemMapAddress 0x00000900
%define PageDirAddress 0x80000
%define PageTableAddress1 0x81000
%define PageTableAddress2 0x82000
%define PageTableAddress3 0x83000
%define StackLoc 0x00080000
%define StackLocVirtualAddress 0xF0001000

mmap_entries dw 0x0000
memSize dd 0x00000000

;GDT--------------------------------------
BGDT:
	dd 0x00000000
	dd 0x00000000
	;code
	dw 0xFFFF ;seg length low word
	dw 0x0000 ;base address low word
	db 0x00 ;base address mid byte
	db 10011010b ;type etc
	db 11001111b ;flags and length high nibble
	db 0x00 ;base address high byte
	
	;data
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
	;load mem map
	mov ax, 0x0000
	mov es, ax
	mov edi, MemMapAddress
	call do_e820
	;map entry count is in bp
	mov word [mmap_entries], bp
	
	;load GDT
	lgdt [GDTInfo]
	cli
	mov eax, cr0
	or al, 00000001b
	mov cr0, eax ;we are now in pmode
	jmp 0x08:ProtectedMode ;set code segment
	
;************** Protected mode *******************
bits 32

ProtectedMode:
	;set up segment registers (cs is already done from the jump to ProtectedMode)
	mov ax, 0x0010 ;data descriptor
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
	mov esp, StackLoc
	
	;enable a20
	in al, 0x92
	or al, 2
	out 0x92, al
	
	;copy kernel
	mov esi, KernelAddress
	mov edi,  NewKernelAddress
	mov ecx, 0x00019000 ;size of kernel (400KiB max) / 4
	cld
	rep movsd
	
	;paging
	call setup_paging
	
	mov esp, StackLocVirtualAddress ;now that paging is enabled...
	
	;Execute Kernel
	;push args onto stack in reverse order
	movzx edx, word [mmap_entries]
	push edx
	mov edx, dword MemMapAddress
	push edx
	;call kernel
	mov ebx, KernelVirtAddress
	call ebx
	
	cli
	hlt

bits 16
; use the INT 0x15, eax= 0xE820 BIOS function to get a memory map
; inputs: es:di -> destination buffer for 24 byte entries
; outputs: bp = entry count, trashes all registers except esi
do_e820:
	xor ebx, ebx		; ebx must be 0 to start
	xor bp, bp		; keep an entry count in bp
	mov edx, dword 0x534D4150	; Place "SMAP" into edx
	mov eax, 0xE820
	mov [es:di + 20], dword 1	; force a valid ACPI 3.X entry
	mov ecx, 24		; ask for 24 bytes
	int 0x15
	jc short .failed	; carry set on first call means "unsupported function"
	mov edx, dword 0x0534D4150	; Some BIOSes apparently trash this register?
	cmp eax, edx		; on success, eax must have been reset to "SMAP"
	jne short .failed
	test ebx, ebx		; ebx = 0 implies list is only 1 entry long (worthless)
	je short .failed
	jmp short .jmpin
.e820lp:
	mov eax, 0xe820		; eax, ecx get trashed on every int 0x15 call
	mov [es:di + 20], dword 1	; force a valid ACPI 3.X entry
	mov ecx, 24		; ask for 24 bytes again
	int 0x15
	jc short .e820f		; carry set means "end of list already reached"
	mov edx, dword 0x0534D4150	; repair potentially trashed register
.jmpin:
	jcxz .skipent		; skip any 0 length entries
	cmp cl, 20		; got a 24 byte ACPI 3.X response?
	jbe short .notext
	test byte [es:di + 20], 1	; if so: is the "ignore this data" bit clear?
	je short .skipent
.notext:
	mov ecx, [es:di + 8]	; get lower dword of memory region length
	or ecx, [es:di + 12]	; "or" it with upper dword to test for zero
	jz .skipent		; if length qword is 0, skip entry
	inc bp			; got a good entry: ++count, move to next storage spot
	add di, 24
.skipent:
	test ebx, ebx		; if ebx resets to 0, list is complete
	jne short .e820lp
.e820f:
	clc			; there is "jc" on end of list to this point, so the carry must be cleared
	ret
.failed:
	stc			; "function unsupported" error exit
	ret
;************************************************************************
bits 32

setup_paging:
	xor ecx, ecx
	xor eax, eax
	mov edi, PageDirAddress
	
.loop_pg:
	mov dword [edi], eax
	inc ecx
	add edi, 4
	cmp ecx, 0x1000 ;0x4000 / 4
	jb .loop_pg
	
	;put page tables into page dir
	mov eax, PageDirAddress
	mov dword [eax], PageTableAddress1 + 1 ;first page dir entry (+1 sets present bit)
	lea ebx, [eax+3072]
	mov dword [ebx], PageTableAddress2 + 1
	
	;identity map first 4mb
	mov ebx, PageTableAddress1
	mov eax, 1 ;address 0 + 1 for present bit
.loop_pg2:
	mov dword [ebx], eax
	add ebx, 4
	add eax, 4096 ;size of one page
	cmp eax, 0x400001 ;4mb + 1 for present bit
	jb .loop_pg2
	
	;map physical kernel address to 0xC0000000 (kernel at 3GiB virtual)
	mov ebx, PageTableAddress2
	mov eax, NewKernelAddress + 1 ;kernel address + 1 for present bit
.loop_pg3:
	mov dword [ebx], eax
	add ebx, 4
	add eax, 4096 ;size of one page
	cmp eax, 0x01400001 ;4MiB + 1 for present bit
	jb .loop_pg3
	
	;map stack to 0xF0001000
	mov eax, StackLocVirtualAddress - 0x1000 ;since stack goes downwards, we want to map the page just before the stack loc
	shr eax, 22
	and eax, 0x3ff ;lowest 10 bits
	lea ebx, [PageDirAddress + (eax * 4)]
	mov dword [ebx], PageTableAddress3 + 1 ;+1 sets present bit
	
	mov eax, StackLocVirtualAddress - 0x1000
	shr eax, 12
	and eax, 0x3ff ;lowest 10 bits
	lea ebx, [PageTableAddress3 + eax*4]
	mov eax, StackLoc - 0x1000 + 1 ;kernel address + 1 for present bit - 0x1000 coz stack grows down
	mov dword [ebx], eax
	
	;enable paging
	mov eax, PageDirAddress
	mov cr3, eax
	mov eax, cr0
	or eax, 0x80000000 ;set paging bit
	mov cr0, eax ;paging is now enabled!
	ret
