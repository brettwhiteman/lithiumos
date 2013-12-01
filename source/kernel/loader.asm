bits 32

global loader

extern kmain
extern sbss
extern ebss

loader:
	;zero bss
	mov ebx, sbss
zloop:
	mov [ebx], byte 0
	inc ebx
	cmp ebx, ebss
	jb zloop

	mov ax, 0x10
	mov ss, ax
	mov esp, 0xF0001000

	jmp kmain

	cli
.hang:
	hlt
	jmp .hang
