/*
Lithium OS text mode printing functions.
*/

#include <print.h>

static uint32_t curX = 0, curY = 0;
static uint8_t colour = 0x07;
static void* ptrVidMem = (void*)0x000B8000;

void print_char(char c)
{
	char *p = (char *)ptrVidMem;

	switch(c)
	{
		case '\n':
			curY++;
			curX = 0;
			break;
		
		case '\b':
			p += curY * SCREEN_COLS * 2;
			p += curX * 2;
			p -= 2;
			p[0] = ' ';
			p[1] = colour;

			if(curX == 0)
			{
				if(curY != 0)
				{
					curY--;
					curX = SCREEN_COLS - 1;
				}
				
			}
			else
				curX--;

			break;
		
		case 27: //esc
			clear_screen();

			break;
			
		default:
			p += curY * SCREEN_COLS * 2;
			p += curX * 2;
			p[0] = c;
			p[1] = colour;
			curX++;

			break;
	}
	
	if(curX >= SCREEN_COLS)
	{
		curX = 0;
		curY++;
	}

	if(curY >= SCREEN_ROWS)
	{
		//TODO: Make it scroll instead of overwriting
		curY = 0;
	}
}

void print_string(const char *s)
{
	uint32_t count = 0;

	while(1)
	{
		if(s[count] == '\0')
			break;

		print_char(s[count]);

		count++;
	}

	update_cursor_pos();
}

void print_string_at(const char *s, uint32_t x, uint32_t y)
{
	curX = x;
	curY = y;

	print_string(s);
}

void clear_screen(void)
{
	curX = 0;
	curY = 0;

	for(uint32_t i = 0; i < SCREEN_COLS * SCREEN_ROWS; i++)
		print_char(' ');

	update_cursor_pos();
}

inline void set_colour(uint8_t c)
{
	colour = c;
}

void update_cursor_pos(void)
{
	uint16_t temp = (curY * SCREEN_COLS) + curX;
    
	outportb(0x3D4, 14);
	outportb(0x3D5, temp >> 8);
	outportb(0x3D4, 15);
	outportb(0x3D5, temp & 0xFF);
}

void set_vid_mem(void *vmem)
{
	ptrVidMem = vmem;
}

inline uint8_t get_colour(void)
{
	return colour;
}

void hide_cursor(void)
{
	uint16_t temp = SCREEN_ROWS * SCREEN_COLS;
    
	outportb(0x3D4, 14);
	outportb(0x3D5, temp >> 8);
	outportb(0x3D4, 15);
	outportb(0x3D5, temp & 0xFF);
}
