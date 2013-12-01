/*
Lithium OS keyboard setup/handling functions.
*/

#include <keyboard.h>
#include <stdinc.h>
#include <print.h>

static bool _key_shift = 0;
//static bool _key_control = 0;
//static bool _key_alt = 0;
static bool _capslock = 0;

static byte kbdus[128] =
{
	0,  27, '1', '2', '3', '4', '5', '6', '7', '8',	/* 9 */
	'9', '0', '-', '=', '\b',	/* Backspace */
	'\t',			/* Tab */
	'q', 'w', 'e', 'r',	/* 19 */
	't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',	/* Enter key */
	0,			/* 29   - Control */
	'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',	/* 39 */
	'\'', '`',   2,		/* Left shift */
	'\\', 'z', 'x', 'c', 'v', 'b', 'n',			/* 49 */
	'm', ',', '.', '/',   3,				/* Right shift */
	'*',
	0,	/* Alt */
	' ',	/* Space bar */
	1,	/* Caps lock */
	0,	/* 59 - F1 key ... > */
	0,   0,   0,   0,   0,   0,   0,   0,
	0,	/* < ... F10 */
	0,	/* 69 - Num lock*/
	0,	/* Scroll Lock */
	0,	/* Home key */
	0,	/* Up Arrow */
	0,	/* Page Up */
	'-',
	0,	/* Left Arrow */
	0,
	0,	/* Right Arrow */
	'+',
	0,	/* 79 - End key*/
	0,	/* Down Arrow */
	0,	/* Page Down */
	0,	/* Insert Key */
	0,	/* Delete Key */
	0,   0,   0,
	0,	/* F11 Key */
	0,	/* F12 Key */
	0,	/* All other keys are undefined */
};

void keyboard_handler(isr_t *stk)
{
	byte scancode;

	/* Read from the keyboard's data buffer */
	scancode = inportb(0x60);
	/* If the top bit of the byte we read from the keyboard is
	*  set, that means that a key has just been released */
	if (scancode & 0x80)
	{
		/* You can use this one to see if the user released the
		*  shift, alt, or control keys... */
		scancode = scancode & 0x7F; //unset highest bit
		switch(kbdus[scancode])
		{
			case 2: //left shift
				_key_shift = 0;
				break;
			
			default:
				break;
		}
	}
	else
	{
		/* Here, a key was just pressed. Please note that if you
		*  hold a key down, you will get repeated key press
		*  interrupts. */

		/* Just to show you how this works, we simply translate
		*  the keyboard scancode into an ASCII value, and then
		*  display it to the screen. You can get creative and
		*  use some flags to see if a shift is pressed and use a
		*  different layout, or you can add another 128 entries
		*  to the above layout to correspond to 'shift' being
		*  held. If shift is held using the larger lookup table,
		*  you would add 128 to the scancode when you look for it */
		byte keycode = kbdus[scancode];
		
		switch(keycode)
		{
			case 1: //caps lock
				if(_capslock)
					_capslock = 0;
				else
					_capslock = 1;
				break;
			
			case 2: //left shift
				_key_shift = 1;
				break;
			
			default:
				print_char(kbdus[scancode]);
				update_cursor_pos();
				break;
		}
	}
}

void keyboard_install(void)
{
	irq_install_handler(1, keyboard_handler);
}
