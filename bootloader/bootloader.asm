bits 16
org 0x7C00

jmp bootloader_start ;jump over data

times 3-($-$$) db 0x90

;BPB START
bpbOEMIdentifier db 'LiOS',0x00,0x00,0x00,0x00 ;OEM identifier
bpbBytesPerSector dw 512 ;bytes per sector
bpbSectorsPerCluster db 8 ;sectors per cluster
bpbReservedSectors dw 1 ;number of reserved sectors
bpbNumberOfFATS db 1 ;number of FATs
bpbRootDirectoryEntries dw 16 ;number of root directory entries
bpbSectors dw 32768 ;total number of sectors (16 MiB disk = 512*32768 bytes)
bpbMediaDescriptor db 0xF8 ;media descriptor
bpbSectorsPerFAT dw 16
bpbSectorsPerTrack dw 0 ;sectors per track
bpbNumHeads dw 0 ;number of heads
bpbHiddenSectors dd 0 ;hidden sectors
bpbSectorsBig dd 0 ;number of sectors (only used if disk size > 32MiB)
db 0x80 ;drive number
db 0 ;reserved
db 0x29 ;extended boot signature
dd 0x5301A70F ;serial number
db 'LiOS Disk',0x00,0x00 ;volume label (11 bytes)
db 'FAT16',0x00,0x00,0x00 ;file system type (8 bytes)
;BPB END

stage2LoadAddress dw 0x0700
rootDirBuffer dw 0x8200
fatBuffer dd 0x0000
fatSegment dw 0x6C20
bootdev db 0x80
stage2 db 'STAGE2  BIN'
kernel db 'KERNEL  BIN'
noFileError db 'File missing', 0x00
loadSectorError db 'Err loading sector. Err: ', 0x00
datasector dw 0x0000
hexc db '0123456789ABCDEF'

bootloader_start:
	mov [bootdev], dl
	xor ax, ax
	cli ;clear interrupts while changing stack
	mov ss, ax ;set stack segment
	mov esp, 0x7C00 ;set stack pointer to just below bootloader
	sti ;restore interrupts
	mov ds, ax ;make sure the data segment is 0
	mov ax, word [fatSegment]
	mov fs, ax ;segment for accessing the FAT in memory


	;Calculate location of root dir
	mov ax, [bpbNumberOfFATS]
	mul word [bpbSectorsPerFAT]
	add ax, word [bpbReservedSectors]
	push ax

	;calculate first data sector
	mov ax, [bpbRootDirectoryEntries]
	mov cx, 32
	mul cx
	add ax, [bpbBytesPerSector]
	dec ax ; add (bytes per sector - 1)
	xor dx, dx
	div word [bpbBytesPerSector]
	pop dx
	push ax
	push dx
	add ax, dx
	mov word [datasector], ax ;first data sector is now in ax

	;load root dir
	xor ecx, ecx
	pop cx ;first sector
	pop ax ;root dir sector count
	xor dx, dx
	mov bx, word [rootDirBuffer]
	call load_sectors

	;Load FAT
	movzx ecx, word [bpbReservedSectors] ;starting sector
	mov ax, word [bpbSectorsPerFAT] ;sectors to load
	mov dx, fs ;buffer segment
	mov bx, [fatBuffer] ;buffer offset
	call load_sectors

	;load stage 2
	push word 0 ;bufSegment
	push word [stage2LoadAddress] ;bufOffset
	push word stage2 ;ptrFilename
	call load_file

	;load kernel
	push word 0 ;bufSegment
	push word [rootDirBuffer] ;bufOffset
	push word kernel ;ptrFilename
	call load_file

	jmp 0x0000:0x0700 ;stage 2




;void load_file(uint16_t ptrFilename, uint16_t bufOffset, uint16_t bufSegment)
load_file:
	mov bx, sp
	lea ebp, [ss:bx]
	mov cx, word [bpbRootDirectoryEntries]
	mov di, word [rootDirBuffer]
	mov si, word [ebp + 2]
.loop_find_file:
	push cx
	mov cx, 11
	push di
	push si
	rep cmpsb
	pop si
	pop di
	je found_file
	pop cx
	add di, 32 ;go to next entry
	loop .loop_find_file
	jmp no_file

found_file:
	pop cx ;clean stack up
	mov ax, word [di + 0x1A] ;starting cluster of file
	mov di, ax ;store cluster in di

	;load cluster
.loop_load_cluster:
	mov ax, di
	sub ax, 2 ;first 2 FAT entries are reserved
	movzx dx, byte [bpbSectorsPerCluster]
	mul word dx
	add ax, word [datasector]
	movzx ecx, ax ;first sector to load in cx
	mov dx, word [ebp + 6]
	mov bx, word [ebp + 4]
	movzx ax, byte [bpbSectorsPerCluster] ;load 1 cluster

	call load_sectors

	mov ax, di
	shl ax, 1 ;multiply by 2 since entry is 16 bits
	add ax, [fatBuffer]
	mov bx, ax
	mov dx, [fs:bx]
	cmp dx, 0xFFF8
	jae load_file_finished
	mov di, dx
	mov ax, [bpbSectorsPerCluster]
	mul word [bpbBytesPerSector]
	xor dx, dx
	mov cx, 16
	div cx
	add word [ebp + 6], ax
	jmp .loop_load_cluster


load_file_finished:
	ret



no_file:
	mov si, noFileError
	call print
	cli
	hlt

print:
	mov ah, 0x0E
	lodsb
	cmp al, byte 0x00
	je print_finished
	int 0x10
	jmp print

print_finished:
	ret

;hex to print in dl
print_hex:
	xor ax, ax
	mov al, dl
	shr al, 4
	mov bx, hexc
	add bx, ax
	mov al, byte [bx]
	mov ah, 0x0E
	int 0x10
	xor ax, ax
	mov al, dl
	and al, 0x0F
	mov bx, hexc
	add bx, ax
	mov al, byte [bx]
	mov ah, 0x0E
	int 0x10
	ret

;Loads a sector from disk.
;Input:
;DX:BX = Buffer address
;ECX = Starting sector to load
;AX = Number of sectors to load
load_sectors:
	push bp
	mov bp, sp
	sub sp, 16 ;align stack to 16 byte boundary
	and sp, 0xFFF0
	;set up DAP structure on stack
	push dword 0 ; LBA high
	push dword ecx ; LBA low
	push word dx ; buffer segment
	push word bx ; buffer offset
	push word ax ; sector count
	push word 0x0010 ; reserved + DAP size
	mov si, sp
	mov dl, [bootdev] ; device
	mov ah, 0x42
	int 0x13 ;load the sector(s)
	jc load_sector_error
	mov sp, bp
	pop bp
	ret
	
load_sector_error:
	mov al, ah
	xor ah, ah
	push ax
	mov si, loadSectorError
	call print
	pop dx
	call print_hex
	cli
	hlt

times 510-($-$$) db 0
db 0x55, 0xAA
