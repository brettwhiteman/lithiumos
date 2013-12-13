/*
Lithium OS panic screen
*/

#include <panic.h>
#include <print.h>
#include <util.h>

const char *exception_messages[] = 
{
	"Division by zero",
	"Debug",
	"Non maskable interrupt",
	"Breakpoint",
	"Into detected overflow",
	"Out of bounds",
	"Invalid opcode",
	"No coprocessor",
	"Double fault",
	"Coprocessor segment overrun",
	"Bad TSS",
	"Segment not present",
	"Stack fault",
	"General protection fault",
	"Page fault",
	"Unknown interrupt",
	"Coprocessor fault",
	"Alignment check",
	"Machine check",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
};

void panic_display_message(isr_t *stk)
{
	//Red on black
	set_colour(0x04);

	clear_screen();

	// Print OS name
	print_string_at("Lithium OS", 34, 5);

	// Print exception message
	print_string_at(exception_messages[stk->int_no], 18, 8);
	print_string(" exception. System halted.");

	char buf[12] = {0};

	// Print error code
	print_string_at("Error code: ", 18, 10);
	itoa((int)stk->err_code, buf, 16);
	print_string(buf);

	// Print register values
	print_string_at("EIP: ", 18, 11);
	itoa((int)stk->eip, buf, 16);
	print_string(buf);

	// Hide cursor
	hide_cursor();

	// Halt system
	disable_interrupts();
	halt_cpu();
}
