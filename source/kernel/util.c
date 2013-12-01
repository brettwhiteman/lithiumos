/*
Lithium OS utilities.
*/

#include <util.h>

static char* numberChars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

void* memset(byte* ptr, byte value, uint32_t count)
{
	for(uint32_t i = 0; i < count; i++)
	{
		ptr[i] = value;
	}

	return (void*)ptr;
}

void* memsetd(uint32_t* ptr, uint32_t value, uint32_t count)
{
	for(uint32_t i = 0; i < count; i++)
	{
		ptr[i] = value;
	}

	return (void*)ptr;
}

inline byte inportb(uint16_t port)
{
	byte ret;

	__asm__ __volatile__ ("inb %%dx,%%al" : "=a" (ret) : "d" (port));

	return ret;
}

inline uint16_t inportw(uint16_t port)
{
	uint16_t ret;

	__asm__ __volatile__ ("inw %%dx,%%ax" : "=a" (ret) : "d" (port));

	return ret;
}

inline void outportb(uint16_t port, byte data)
{
	__asm__ __volatile__ ("outb %%al,%%dx" : : "d" (port), "a" (data));
}

void itoa(uint32_t number, char *buf, uint32_t base)
{
	// Sanity check
	if((base == 0) || (base > 36))
		return;

	char buf2[16] = {0};
	uint32_t num = 0;
	uint32_t c = 0;
	uint32_t c2 = 0;
	
	while(number != 0)
	{
		num = number % base;
		number /= base;
		buf2[c] = numberChars[num];
		c++;
	}
	
	c2 = c;
	
	for(uint32_t i = 0; i < c2; i++)
	{
		c--;
		buf[i] = buf2[c];
	}
	
	buf[c2] = 0;
	
	if(buf[0] == 0)
	{
		buf[0] = '0';
		buf[1] = 0;
	}
}

inline void disable_interrupts(void)
{
	__asm__ __volatile__("cli");
}

inline void enable_interrupts(void)
{
	__asm__ __volatile__("sti");
}

inline void halt_cpu(void)
{
	__asm__ __volatile__("hlt");
}

uint32_t strcmp(char* string1, char* string2)
{
	uint32_t i = 0;

	while(string1[i] == string2[i])
	{
		if(string1[i++] == 0)
		{
			return 0;
		}
	}

	return i + 1;
}

void memcpy(void *dest, const void *source, size_t num)
{
	for(uint32_t i = 0; i < num; ++i)
	{
		((uint8_t *)dest)[i] = ((uint8_t *)source)[i];
	}
}
