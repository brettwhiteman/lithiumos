/*
Lithium OS utilities.
*/

#include <util.h>

void *memset(void *ptr, int value, size_t count)
{
	uint8_t *p = (uint8_t *)ptr;

	for(uint32_t i = 0; i < count; i++)
	{
		p[i] = (uint8_t)value;
	}

	return ptr;
}

void* memsetd(uint32_t *ptr, uint32_t value, size_t count)
{
	for(uint32_t i = 0; i < count; i++)
	{
		ptr[i] = value;
	}

	return (void *)ptr;
}

inline uint8_t inportb(uint16_t port)
{
	uint8_t ret;

	__asm__ __volatile__ ("inb %%dx,%%al" : "=a" (ret) : "d" (port));

	return ret;
}

inline uint16_t inportw(uint16_t port)
{
	uint16_t ret;

	__asm__ __volatile__ ("inw %%dx,%%ax" : "=a" (ret) : "d" (port));

	return ret;
}

inline void outportb(uint16_t port, uint8_t data)
{
	__asm__ __volatile__ ("outb %%al,%%dx" : : "d" (port), "a" (data));
}

char *itoa(int value, char *str, int base)
{
	if(value == 0)
	{
		str[0] = '0';
		str[1] = 0;

		return str;
	}

	bool negative = FALSE;

	if((value < 0) && (base == 10))
	{
		negative = TRUE;
		value *= -1;
	}

	uint32_t val = (uint32_t)value;
	uint32_t i = 0;

	while(val != 0)
	{
		uint32_t rem = val % base;
		val /= base;
		str[i++] = (rem > 9) ? ('A' + rem - 10) : ('0' + rem);
	}

	if(negative)
		str[i++] = '-';

	str[i] = 0;

	strrev(str);

	return str;
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

uint32_t strcmp(char *string1, char *string2)
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

size_t strlen(const char *str)
{
	size_t len = 0;

	while(str[len] != 0)
		++len;

	return len;
}

void strrev(char *str)
{
	uint32_t start = 0;
	uint32_t end = strlen((const char *)str) - 1;
	char temp = 0;

	while(start < end)
	{
		temp = str[end];
		str[end] = str[start];
		str[start] = temp;
		++start;
		--end;
	}
}
