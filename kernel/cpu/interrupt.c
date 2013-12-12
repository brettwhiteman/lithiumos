/*
Lithium OS interrupt setup/handling functions.
*/

#include <interrupt.h>
#include <pagefault.h>
#include <panic.h>

struct idtInfo
{
	uint16_t limit;
	uint32_t base_address;
} __attribute__((__packed__));

struct idtDescriptor
{
   uint16_t offset_1; // offset bits 0..15
   uint16_t selector; // a code segment selector in GDT or LDT
   uint8_t zero;      // unused, set to 0
   uint8_t type_attr; // type and attributes
   uint16_t offset_2; // offset bits 16..31
} __attribute__((__packed__));

static struct idtDescriptor idt[256];

void set_idt(struct idtInfo* idti)
{
	__asm("lidt (%0)" : : "b" (idti));
}

void setup_interrupts(void)
{
	struct idtInfo idti;
	idti.limit = ((sizeof(struct idtDescriptor) * 256) - 1);
	idti.base_address = (uint32_t)&idt;
	memset((uint8_t *)&idt, 0, (sizeof(struct idtDescriptor) * 256));
	set_idt(&idti);
	install_isrs();
	install_irqs();
}

void register_interrupt(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags)
{
	idt[num].offset_1 = base & 0x0000FFFF;
	idt[num].offset_2 = base >> 16;
	idt[num].selector = sel;
	idt[num].zero = 0;
	idt[num].type_attr = flags;
}

void fault_handler(isr_t *stk)
{
	if(stk->int_no < 32)
	{
		if(stk->int_no == 14) // Page fault
		{
			pagefault_handle(stk);

			// Send EOI to master interrupt controller
			outportb(0x20, 0x20);
		}
		else
		{	
			panic_display_message(stk);
		}
	}
}

extern void _isr0();
extern void _isr1();
extern void _isr2();
extern void _isr3();
extern void _isr4();
extern void _isr5();
extern void _isr6();
extern void _isr7();
extern void _isr8();
extern void _isr9();
extern void _isr10();
extern void _isr11();
extern void _isr12();
extern void _isr13();
extern void _isr14();
extern void _isr15();
extern void _isr16();
extern void _isr17();
extern void _isr18();
extern void _isr19();
extern void _isr20();
extern void _isr21();
extern void _isr22();
extern void _isr23();
extern void _isr24();
extern void _isr25();
extern void _isr26();
extern void _isr27();
extern void _isr28();
extern void _isr29();
extern void _isr30();
extern void _isr31();

void install_isrs(void)
{
	register_interrupt(0,  (uint32_t)_isr0,  0x08, 0x8E);
	register_interrupt(1,  (uint32_t)_isr1,  0x08, 0x8E);
	register_interrupt(2,  (uint32_t)_isr2,  0x08, 0x8E);
	register_interrupt(3,  (uint32_t)_isr3,  0x08, 0x8E);
	register_interrupt(4,  (uint32_t)_isr4,  0x08, 0x8E);
	register_interrupt(5,  (uint32_t)_isr5,  0x08, 0x8E);
	register_interrupt(6,  (uint32_t)_isr6,  0x08, 0x8E);
	register_interrupt(7,  (uint32_t)_isr7,  0x08, 0x8E);
	register_interrupt(8,  (uint32_t)_isr8,  0x08, 0x8E);
	register_interrupt(9,  (uint32_t)_isr9,  0x08, 0x8E);
	register_interrupt(10, (uint32_t)_isr10, 0x08, 0x8E);
	register_interrupt(11, (uint32_t)_isr11, 0x08, 0x8E);
	register_interrupt(12, (uint32_t)_isr12, 0x08, 0x8E);
	register_interrupt(13, (uint32_t)_isr13, 0x08, 0x8E);
	register_interrupt(14, (uint32_t)_isr14, 0x08, 0x8E);
	register_interrupt(15, (uint32_t)_isr15, 0x08, 0x8E);
	register_interrupt(16, (uint32_t)_isr16, 0x08, 0x8E);
	register_interrupt(17, (uint32_t)_isr17, 0x08, 0x8E);
	register_interrupt(18, (uint32_t)_isr18, 0x08, 0x8E);
	register_interrupt(19, (uint32_t)_isr19, 0x08, 0x8E);
	register_interrupt(20, (uint32_t)_isr20, 0x08, 0x8E);
	register_interrupt(21, (uint32_t)_isr21, 0x08, 0x8E);
	register_interrupt(22, (uint32_t)_isr22, 0x08, 0x8E);
	register_interrupt(23, (uint32_t)_isr23, 0x08, 0x8E);
	register_interrupt(24, (uint32_t)_isr24, 0x08, 0x8E);
	register_interrupt(25, (uint32_t)_isr25, 0x08, 0x8E);
	register_interrupt(26, (uint32_t)_isr26, 0x08, 0x8E);
	register_interrupt(27, (uint32_t)_isr27, 0x08, 0x8E);
	register_interrupt(28, (uint32_t)_isr28, 0x08, 0x8E);
	register_interrupt(29, (uint32_t)_isr29, 0x08, 0x8E);
	register_interrupt(30, (uint32_t)_isr30, 0x08, 0x8E);
	register_interrupt(31, (uint32_t)_isr31, 0x08, 0x8E);
}

extern void _irq0();
extern void _irq1();
extern void _irq2();
extern void _irq3();
extern void _irq4();
extern void _irq5();
extern void _irq6();
extern void _irq7();
extern void _irq8();
extern void _irq9();
extern void _irq10();
extern void _irq11();
extern void _irq12();
extern void _irq13();
extern void _irq14();
extern void _irq15();

void *irq_routines[16] = {0};

void irq_handler(isr_t *stk)
{
	void (*handler)(isr_t *stk);
	
	handler = irq_routines[stk->int_no - 32];
	
	if(handler)
	{
		handler(stk);
	}
	
	// If the IDT entry that was invoked was greater than or
	// equal to 40 (meaning IRQ8 - 15), then we need to send an
	// EOI to the slave controller
	if(stk->int_no >= 40)
	{
		outportb(0xA0, 0x20);
	}

	// In either case, we need to send an EOI to the master
	// interrupt controller too
	outportb(0x20, 0x20);
}

void irq_remap(void)
{
	outportb(0x20, 0x11);
	outportb(0xA0, 0x11);
	outportb(0x21, 0x20);
	outportb(0xA1, 0x28);
	outportb(0x21, 0x04);
	outportb(0xA1, 0x02);
	outportb(0x21, 0x01);
	outportb(0xA1, 0x01);
	outportb(0x21, 0x0);
	outportb(0xA1, 0x0);
}

void install_irqs(void)
{
	irq_remap();

	// Timer
	register_interrupt(32, (uint32_t)_irq0, 0x08, 0x8E);

	// Keyboard
	register_interrupt(33, (uint32_t)_irq1, 0x08, 0x8E);

	// System call IRQ
	register_interrupt(34, (uint32_t)_irq2, 0x08, 0xEE);

	register_interrupt(35, (uint32_t)_irq3, 0x08, 0x8E);
	register_interrupt(36, (uint32_t)_irq4, 0x08, 0x8E);
	register_interrupt(37, (uint32_t)_irq5, 0x08, 0x8E);
	register_interrupt(38, (uint32_t)_irq6, 0x08, 0x8E);
	register_interrupt(39, (uint32_t)_irq7, 0x08, 0x8E);
	register_interrupt(40, (uint32_t)_irq8, 0x08, 0x8E);
	register_interrupt(41, (uint32_t)_irq9, 0x08, 0x8E);
	register_interrupt(42, (uint32_t)_irq10, 0x08, 0x8E);
	register_interrupt(43, (uint32_t)_irq11, 0x08, 0x8E);
	register_interrupt(44, (uint32_t)_irq12, 0x08, 0x8E);
	register_interrupt(45, (uint32_t)_irq13, 0x08, 0x8E);
	register_interrupt(46, (uint32_t)_irq14, 0x08, 0x8E);
	register_interrupt(47, (uint32_t)_irq15, 0x08, 0x8E);
}

void irq_install_handler(uint32_t irq, void (*handler)(isr_t *stk))
{
	irq_routines[irq] = handler;
}

void irq_uninstall_handler(uint32_t irq)
{
	irq_routines[irq] = 0;
}
