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

;DAP for extended read
dapStructure:
	db 0x10 ;size
	db 0x00 ;unused
	dw 0x0000 ;sectors to read
	dd 0x00000000 ;segment:offset pointer to memory buffer
	dd 0x00000000 ;LBA address of sector low
	dd 0x00000000 ;LBA address of sector high
;END DAP

stage2LoadAddress dw 0x0700
rootDirBuffer dw 0x8200
fatBuffer dd 0x0000
fatSegment dw 0x6C20
bootdev db 0x80
stage2 db 'STAGE2  BIN'
kernel db 'KERNEL  BIN'
noFileError db 'File(s) missing', 0x00
loadSectorError db 'Error loading sector(s) Err: ', 0x00
datasector dw 0x0000
hexc db '0123456789ABCDEF'

bootloader_start:
	mov [bootdev], dl
	mov ax, 0x07E0 ;set up stack space
	cli ;clear interrupts while changing stack
	mov ss, ax ;set stack segment
	mov sp, 0x400 ;set stack pointer (1KiB of stack)
	sti ;restore interrupts
	xor ax, ax
	mov ds, ax ;make sure the data segment is 0
	mov ax, word [fatSegment]
	mov fs, ax ;segment for accessing the FAT in memory

	;Calculate size of root directory
	mov ax, 32 ;size of 1 root directory entry
	mul word [bpbRootDirectoryEntries]
	mov dx, word [bpbBytesPerSector]
	dec dx
	add ax, dx ;add BytesPerSector - 1 so it rounds up.
	xor dx, dx
	div word [bpbBytesPerSector]
	mov cx, ax ;cx now holds size of root dir (size in sectors)


	;Calculate location of root dir
	mov ax, [bpbNumberOfFATS]
	mul word [bpbSectorsPerFAT]
	add ax, word [bpbReservedSectors]
	;store location - it is the first data sector
	mov bx, ax
	add bx, cx
	mov word [datasector], bx

	;Load RootDir
	xor dx, dx
	mov bx, word [rootDirBuffer]
	xchg al, cl
	call load_sector

	;Load FAT
	mov cx, word [bpbReservedSectors] ;location of FAT
	xor ax, ax
	mov al, byte [bpbNumberOfFATS]
	mul word [bpbSectorsPerFAT] ;al now contains sector count
	mov dx, fs
	mov bx, [fatBuffer]
	call load_sector

	;load stage 2
	mov ax, word [stage2LoadAddress]
	push ax
	mov ax, stage2
	push ax
	call load_file

	;load kernel
	mov ax, word [rootDirBuffer] ;kernel goes where root dir was
	push ax
	mov ax, kernel
	push ax
	call load_file
	
	jmp 0x0000:0x0700 ;execute Stage2
	
	cli
	hlt




;void load_file(char *filename, void *buf)
load_file:
	mov cx, [bpbRootDirectoryEntries]
	mov di, [rootDirBuffer]
	mov si, [esp + 2]
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
	push ax ;store cluster on stack

	;load cluster
.loop_load_cluster:
	mov bx, [esp + 6]
	mov ax, word [ss:esp] ;get cluster into ax
	sub ax, 2 ;first 2 FAT entries are reserved
	movzx dx, byte [bpbSectorsPerCluster]
	mul word dx
	add ax, word [datasector]
	mov cx, ax ;first sector to load in cx
	xor dx, dx ;segment in dx (0x0)
	mov al, byte [bpbSectorsPerCluster] ;load 1 cluster
	call load_sector

	pop ax
	shl ax, 1 ;multiply by 2 since entry is 16 bits
	add ax, [fatBuffer]
	mov bx, ax
	mov dx, word [fs:bx]
	cmp dx, 0xFFF8
	jae load_file_finished
	push dx
	mov ax, [bpbSectorsPerCluster]
	mul word [bpbBytesPerSector]
	add word [esp + 6], ax
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
;CL = Starting sector to load
;AL = Number of sectors to load
load_sector:
	mov byte [dapStructure + 2], al
	mov word [dapStructure + 4], bx
	mov word [dapStructure + 6], dx
	mov byte [dapStructure + 8], cl
	mov dl, [bootdev] ; device
	mov si, dapStructure
	mov ah, 0x42
	int 0x13 ;load the sector(s)
	jc load_sector_error
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
