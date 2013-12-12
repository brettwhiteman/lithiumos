#ifndef INTERRUPT_H
#define INTERRUPT_H

#include <stdinc.h>

typedef struct isr_struct
{
	uint32_t gs, fs, es, ds;      //Pushed the segment registers last
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;  //Pushed by 'pushad'
	uint32_t int_no, err_code;    //Our 'push byte #' and ecodes do this
	uint32_t eip, cs, eflags, useresp, ss;   //Pushed by the processor automatically
} isr_t;

struct idtInfo;

void set_idt(struct idtInfo* idti);
void setup_interrupts(void);
void register_interrupt(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags);
void fault_handler(isr_t *stk);
void install_isrs(void);
void irq_handler(isr_t *stk);
void irq_remap(void);
void install_irqs(void);
void irq_install_handler(uint32_t irq, void (*handler)(isr_t* stk));
void irq_uninstall_handler(uint32_t irq);

#endif
