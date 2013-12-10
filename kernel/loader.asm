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

	jmp kmain

	cli
.hang:
	hlt
	jmp .hang
