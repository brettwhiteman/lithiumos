/*
Lithium OS keyboard setup/handling functions.
*/

#include <keyboard.h>
#include <stdinc.h>
#include <print.h>

static bool keyShift = 0;
static bool capsLockOn = 0;

static const char kbdus[128] =
{
	0,  27, '1', '2', '3', '4', '5', '6', '7', '8',
	'9', '0', '-', '=', '\b', // Backspace
	'\t', // Tab
	'q', 'w', 'e', 'r',
	't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', // Enter
	0, // 29 - Control
	'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',
	'\'', '`',   2, // Left Shift
	'\\', 'z', 'x', 'c', 'v', 'b', 'n',
	'm', ',', '.', '/',   3, // Right Shift
	'*',
	0, // Alt
	' ', // Space
	1, // Caps Lock
	0, // F1 ...
	0,   0,   0,   0,   0,   0,   0,   0,
	0, // F10
	0, // Num Lock
	0, // Scroll Lock
	0, // Home
	0, // Up Arrow
	0, // Page Up
	'-',
	0, // Left Arrow
	0,
	0, // Right Arrow
	'+',
	0, // End
	0, // Down
	0, // Page Down
	0, // Insert
	0, // Delete
	0,   0,   0,
	0, // F11
	0, // F12
	0, // All other keys are undefined
};

static const char kbdusUppercase[128] =
{
	0,  27, '!', '@', '#', '$', '%', '^', '&', '*',
	'(', ')', '_', '+', '\b', // Backspace
	'\t', // Tab
	'Q', 'W', 'E', 'R',
	'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', // Enter
	0, // 29 - Control
	'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',
	'\"', '~',   2, // Left Shift
	'|', 'Z', 'X', 'C', 'V', 'B', 'N',
	'M', '<', '>', '?',   3, // Right Shift
	'*',
	0, // Alt
	' ', // Space
	1, // Caps Lock
	0, // F1 ...
	0,   0,   0,   0,   0,   0,   0,   0,
	0, // F10
	0, // Num Lock
	0, // Scroll Lock
	0, // Home
	0, // Up Arrow
	0, // Page Up
	'-',
	0, // Left Arrow
	0,
	0, // Right Arrow
	'+',
	0, // End
	0, // Down
	0, // Page Down
	0, // Insert
	0, // Delete
	0,   0,   0,
	0, // F11
	0, // F12
	0, // All other keys are undefined
};

void keyboard_handler(isr_t *stk)
{
	uint8_t scancode;

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
				keyShift = 0;
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

		char keycode = 0;

		if(keyShift)
			keycode = kbdusUppercase[scancode];
		else
			keycode = kbdus[scancode];
		
		switch(keycode)
		{
			case 0:
				// Do nothing
				break;

			case 1: //caps lock
				capsLockOn = !capsLockOn;
				break;
			
			case 2: //left shift
				keyShift = 1;
				break;
			
			default:
			{
				char buf[2] = {0};
				buf[0] = keycode;
				buf[1] = 0;
				print_string(buf);
				break;
			}
		}
	}
}

void keyboard_install(void)
{
	irq_install_handler(1, keyboard_handler);
}
