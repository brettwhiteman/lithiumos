/*
Lithium OS timer functions. Handles timer IRQs.
*/

#include <timer.h>
#include <scheduler.h>

static uint32_t timer_ticks = 0;

void set_timer_frequency(uint32_t hz)
{
	uint32_t divisor = 1193180 / hz;       /* Calculate our divisor */
	outportb(0x43, 0x36);             /* Set our command byte 0x36 */
	outportb(0x40, divisor & 0xFF);   /* Set low byte of divisor */
	outportb(0x40, divisor >> 8);     /* Set high byte of divisor */
}

void timer_handler(isr_t *stk)
{
	// Increment tick count
	timer_ticks += 5;

	registers_t regs;
	regs.gs = stk->gs;
	regs.fs = stk->fs;
	regs.es = stk->es;
	regs.ds = stk->ds;
	regs.edi = stk->edi;
	regs.esi = stk->esi;
	regs.ebp = stk->ebp;
	regs.esp = stk->esp;
	regs.ebx = stk->ebx;
	regs.edx = stk->edx;
	regs.ecx = stk->ecx;
	regs.eax = stk->eax;
	regs.eip = stk->eip;
	regs.cs = stk->cs;
	regs.eflags = stk->eflags;
	regs.useresp = stk->useresp;
	regs.ss = stk->ss;

	scheduler_tick(&regs);

	stk->gs = regs.gs;
	stk->fs = regs.fs;
	stk->es = regs.es;
	stk->ds = regs.ds;
	stk->edi = regs.edi;
	stk->esi = regs.esi;
	stk->ebp = regs.ebp;
	stk->ebx = regs.ebx;
	stk->edx = regs.edx;
	stk->ecx = regs.ecx;
	stk->eax = regs.eax;
	stk->eip = regs.eip;
	stk->cs = regs.cs;
	stk->eflags = regs.eflags;
	stk->useresp = regs.useresp;
	stk->ss = regs.ss;
}

void timer_install(void)
{
	irq_install_handler(0, timer_handler);
	set_timer_frequency(200);
}

void timer_wait(uint32_t ticks)
{
	uint32_t eticks;

	eticks = timer_ticks + ticks;
	
	while(timer_ticks < eticks);
}

uint32_t get_tick_count(void)
{
	return timer_ticks;
}
