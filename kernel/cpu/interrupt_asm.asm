bits 32

global _isr0
global _isr1
global _isr2
global _isr3
global _isr4
global _isr5
global _isr6
global _isr7
global _isr8
global _isr9
global _isr10
global _isr11
global _isr12
global _isr13
global _isr14
global _isr15
global _isr16
global _isr17
global _isr18
global _isr19
global _isr20
global _isr21
global _isr22
global _isr23
global _isr24
global _isr25
global _isr26
global _isr27
global _isr28
global _isr29
global _isr30
global _isr31

;  0: Divide By Zero Exception
_isr0:
    cli
    push byte 0    ; A normal ISR stub that pops a dummy error code to keep a uniform stack frame
    push byte 0
    jmp isr_common_stub

;  1: Debug Exception
_isr1:
    cli
    push byte 0
    push byte 1
    jmp isr_common_stub
    
; 2:  	Non Maskable Interrupt Exception
_isr2:
    cli
    push byte 0
    push byte 2
    jmp isr_common_stub
    
;3: Breakpoint Exception
_isr3:
    cli
    push byte 0
    push byte 3
    jmp isr_common_stub
    
;4: Into Detected Overflow Exception
_isr4:
    cli
    push byte 0
    push byte 4
    jmp isr_common_stub
    
;5: Out of Bounds Exception
_isr5:
    cli
    push byte 0
    push byte 5
    jmp isr_common_stub
    
;6: Invalid Opcode Exception
_isr6:
    cli
    push byte 0
    push byte 6
    jmp isr_common_stub
    
;7: No Coprocessor Exception
_isr7:
    cli
    push byte 0
    push byte 7
    jmp isr_common_stub

;  8: Double Fault Exception
_isr8:
    cli
    push byte 8        ; Note that we DON'T push a value on the stack in this one!
                   ; It pushes one already! Use this type of stub for exceptions that pop error codes!
    jmp isr_common_stub
    
    
;9: Coprocessor Segment Overrun Exception
_isr9:
    cli
    push byte 0
    push byte 9
    jmp isr_common_stub
    
    
;10: Bad TSS Exception
_isr10:
    cli
    push byte 10
    jmp isr_common_stub
    
    
;11: Segment Not Present Exception
_isr11:
    cli
    push byte 11
    jmp isr_common_stub
    
    
;12: Stack Fault Exception
_isr12:
    cli
    push byte 12
    jmp isr_common_stub
    
    
;13: General Protection Fault Exception
_isr13:
    cli
    push byte 13
    jmp isr_common_stub
    
    
;14: Page Fault Exception
_isr14:
    cli
    push byte 14
    jmp isr_common_stub
    
    
;15: Unknown Interrupt Exception
_isr15:
    cli
    push byte 0
    push byte 15
    jmp isr_common_stub
    
    
;16: Coprocessor Fault Exception
_isr16:
    cli
    push byte 0
    push byte 16
    jmp isr_common_stub
    
    
;17: Alignment Check Exception (486+)
_isr17:
    cli
    push byte 0
    push byte 17
    jmp isr_common_stub
    
    
;18: Machine Check Exception (Pentium/586+)
_isr18:
    cli
    push byte 0
    push byte 18
    jmp isr_common_stub
    
    
;19: Reserved Exception
_isr19:
    cli
    push byte 0
    push byte 19
    jmp isr_common_stub
    
    
;20: Reserved Exception
_isr20:
    cli
    push byte 0
    push byte 20
    jmp isr_common_stub
    
    
;21: Reserved Exception
_isr21:
    cli
    push byte 0
    push byte 21
    jmp isr_common_stub
    
;22: Reserved Exception
_isr22:
    cli
    push byte 0
    push byte 22
    jmp isr_common_stub
    
    
;23: Reserved Exception
_isr23:
    cli
    push byte 0
    push byte 23
    jmp isr_common_stub
    
    
;24: Reserved Exception
_isr24:
    cli
    push byte 0
    push byte 24
    jmp isr_common_stub
    
    
;25: Reserved Exception
_isr25:
    cli
    push byte 0
    push byte 25
    jmp isr_common_stub
    
    
;26: Reserved Exception
_isr26:
    cli
    push byte 0
    push byte 26
    jmp isr_common_stub
    
    
;27: Reserved Exception
_isr27:
    cli
    push byte 0
    push byte 27
    jmp isr_common_stub
    
    
;28: Reserved Exception
_isr28:
    cli
    push byte 0
    push byte 28
    jmp isr_common_stub
    
    
;29: Reserved Exception
_isr29:
    cli
    push byte 0
    push byte 29
    jmp isr_common_stub
    
    
;30: Reserved Exception
_isr30:
    cli
    push byte 0
    push byte 30
    jmp isr_common_stub
    
    
;31: Reserved Exception
_isr31:
    cli
    push byte 0
    push byte 31
    jmp isr_common_stub





; We call a C function in here. We need to let the assembler know
; that 'fault_handler' exists in another file
extern fault_handler

; This is our common ISR stub. It saves the processor state, sets
; up for kernel mode segments, calls the C-level fault handler,
; and finally restores the stack frame.
isr_common_stub:
    pushad
    push ds
    push es
    push fs
    push gs
    mov ax, 0x10   ; Load the Kernel Data Segment descriptor!
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov eax, esp   ; Push us the stack
    push eax
    mov eax, fault_handler
    call eax       ; A special call, preserves the 'eip' register
    pop eax
    pop gs
    pop fs
    pop es
    pop ds
    popad
    add esp, 8     ; Cleans up the pushed error code and pushed ISR number
    iret           ; pops 5 things at once: CS, EIP, EFLAGS, SS, and ESP!


global _irq0
global _irq1
global _irq2
global _irq3
global _irq4
global _irq5
global _irq6
global _irq7
global _irq8
global _irq9
global _irq10
global _irq11
global _irq12
global _irq13
global _irq14
global _irq15


; 32: IRQ0
_irq0:
	cli
	push byte 0
	push byte 32
	jmp irq_common_stub
    
 ; 33: IRQ1 
_irq1:
	cli
	push byte 0
	push byte 33
	jmp irq_common_stub
	
; 34: IRQ2	The switch_task() function takes the saved registers_t as the argument, and should be called by the IRQ0 handler. 
_irq2:
	cli
	push byte 0
	push byte 34
	jmp irq_common_stub
	
; 35: IRQ3	
_irq3:
	cli
	push byte 0
	push byte 35
	jmp irq_common_stub
	
; 36: IRQ4	
_irq4:
	cli
	push byte 0
	push byte 36
	jmp irq_common_stub
	
; 37: IRQ5	
_irq5:
	cli
	push byte 0
	push byte 37
	jmp irq_common_stub
	
; 38: IRQ6	
_irq6:
	cli
	push byte 0
	push byte 38
	jmp irq_common_stub
	
; 39: IRQ7	
_irq7:
	cli
	push byte 0
	push byte 39
	jmp irq_common_stub
	
; 40: IRQ8	
_irq8:
	cli
	push byte 0
	push byte 40
	jmp irq_common_stub
	
; 41: IRQ9	
_irq9:
	cli
	push byte 0
	push byte 41
	jmp irq_common_stub
	
; 42: IRQ10	
_irq10:
	cli
	push byte 0
	push byte 42
	jmp irq_common_stub
	
; 43: IRQ11	
_irq11:
	cli
	push byte 0
	push byte 43
	jmp irq_common_stub
	
; 44: IRQ12
_irq12:
	cli
	push byte 0
	push byte 44
	jmp irq_common_stub
	
; 45: IRQ13
_irq13:
	cli
	push byte 0
	push byte 45
	jmp irq_common_stub
	
; 46: IRQ14
_irq14:
	cli
	push byte 0
	push byte 46
	jmp irq_common_stub
	
; 47: IRQ15
_irq15:
	cli
	push byte 0
	push byte 47
	jmp irq_common_stub
	
extern irq_handler
    
irq_common_stub:
	pushad
	push ds
	push es
	push fs
	push gs
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov eax, esp
	push eax ;push pointer to stack as argument
	mov eax, irq_handler
	call eax
	pop eax
	pop gs
	pop fs
	pop es
	pop ds
	popad
	add esp, 8
	iret
