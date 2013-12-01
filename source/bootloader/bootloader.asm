bits 16
org 0x7C00

jmp bootloader_start ;jump over data

times 3-($-$$) db 0x90

;BPB START
db 'LiOS',0x00,0x00,0x00,0x00 ;OEM identifier
bpbBytesPerSector dw 512 ;bytes per sector
bpbSectorsPerCluster db 1 ;sectors per cluster
bpbReservedSectors dw 1 ;number of reserved sectors
bpbNumberOfFATS db 1 ;number of FATs
bpbRootDirectoryEntries dw 512 ;number of root directory entries
dw 32768 ;number of sectors (16 MiB disk = 512*32768 bytes)
db 0xF8 ;media descriptor
bpbSectorsPerFAT dw 128 ;sectors per FAT
dw 0 ;sectors per track
dw 0 ;number of heads
dd 0 ;hidden sectors
dd 0 ;number of sectors (only used if disk size > 32MiB)
db 0x80 ;drive number
db 0 ;reserved
db 0x29 ;extended boot signature
dd 0x5301A70F ;serial number
db 'LiOS Disk',0x00,0x00 ;volume label
db 'FAT16',0x00,0x00,0x00
;BPB END

;DAP for extended read
DAPStructure:
	db 0x10 ;size
	db 0x00 ;unused
	dw 0x0000 ;sectors to read
	dd 0x00000000 ;segment:offset pointer to memory buffer
	dd 0x00000000 ;LBA address of sector low
	dd 0x00000000 ;LBA address of sector high
;END DAP

Stage2LoadAddress dw 0x0700
FileLoadAddress dw 0x0000
RootDirBuffer dw 0x8200
FATBuffer dd 0x0000
FATSegment dw 0x6C20
bootdev db 0x80
STAGE2 db 'STAGE2  BIN'
KERNEL db 'KERNEL  BIN'
FileName db '           '
NoFileError db 'S', 0x0D, 0x0A, 0x00
cluster dw 0x0000
datasector dw 0x0000
hexc db '0123456789ABCDEF'

bootloader_start:
	mov [bootdev], dl
	mov ax, 0x07E0 ;set up stack space
	cli ;clear interrupts while changing stack
	mov ss, ax ;set stack segReturn: CF clear if successfulment
	mov sp, 0x400 ;set stack pointer (1KiB of stack)
	sti ;restore interrupts
	xor ax, ax
	mov ds, ax ;make sure the data segment is 0
	mov ax, word [FATSegment]
	mov fs, ax ;segment for accessing the FAT in memory

	;Load Stage2 from disk
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
	mov bx, word [RootDirBuffer]
	xchg al, cl
	call loadsector

	;Load FAT
	mov cx, word [bpbReservedSectors] ;location of FAT
	xor ax, ax
	mov al, byte [bpbNumberOfFATS]
	mul word [bpbSectorsPerFAT] ;al now contains sector count
	mov dx, fs
	mov bx, [FATBuffer]
	call loadsector

	;load stage 2
	mov si, STAGE2
	mov di, FileName
	mov cx, 11
	rep movsb
	mov ax, word [Stage2LoadAddress]
	mov word [FileLoadAddress], ax
	call LoadFile

	;load kernel
	mov si, KERNEL
	mov di, FileName
	mov cx, 11
	rep movsb
	mov ax, word [RootDirBuffer] ;kernel goes where root dir was
	mov word [FileLoadAddress], ax
	call LoadFile
	
	jmp 0x0000:0x0700 ;execute Stage2
	cli
	hlt




	
LoadFile:
	mov cx, [bpbRootDirectoryEntries]
	mov di, [RootDirBuffer]
.loop1:
	push cx
	mov cx, 11
	mov si, FileName
	push di
	rep cmpsb
	pop di
	je .done
	pop cx
	add di, 32 ;go to next entry
	loop .loop1
	jmp no_file

.done:
	pop cx ;clean stack up
	mov ax, word [di + 0x1A] ;starting cluster of file
	mov word [cluster], ax

	;load cluster
.loop_load_cluster:
	mov bx, [FileLoadAddress]
	mov ax, word [cluster] ;starting cluster
	sub ax, 2
	movzx dx, byte [bpbSectorsPerCluster]
	mul word dx
	add ax, word [datasector]
	mov cx, ax
	xor dx, dx
	mov al, byte [bpbSectorsPerCluster] ;load 1 cluster
	call loadsector

	mov ax, word [cluster]
	shl ax, 1 ;multiply by 2
	add ax, [FATBuffer]
	mov bx, ax
	mov dx, word [fs:bx]
	cmp dx, 0xFFF8
	jae .finished
	mov word [cluster], dx
	mov ax, [bpbSectorsPerCluster]
	mul word [bpbBytesPerSector]
	add word [FileLoadAddress], ax
	jmp .loop_load_cluster


.finished:
	ret

no_file:
	mov si, NoFileError
	call print
	cli
	hlt

print:
	mov ah,0x0E
	lodsb
	cmp al,byte 0x00
	je finprint
	int 0x10
	jmp print

finprint:
	ret

;hex to print in dl
printhex:
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
loadsector:
	mov byte [DAPStructure + 2], al
	mov word [DAPStructure + 4], bx
	mov word [DAPStructure + 6], dx
	mov byte [DAPStructure + 8], cl
	mov dl, [bootdev] ; device
	mov si, DAPStructure
	mov ah,0x42
	int 0x13 ;load the sector(s)
	jc LoadSectorErr
	ret
	
LoadSectorErr:
	mov dl, ah
	call printhex
	cli
	hlt

times 510-($-$$) db 0
db 0x55, 0xAA