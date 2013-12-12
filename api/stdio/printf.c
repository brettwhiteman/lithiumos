#include <funccodes.h>
#include <stdio.h>
#include <stdarg.h>
#include <types.h>
#include <stdlib.h>

#define PRINTF_BUF_LEN 64

void printf(const char *format, ...)
{
	va_list va;
	va_start(va, format);

	char buf[PRINTF_BUF_LEN] = {0};
	uint32_t bc = 0;

	for(uint32_t i = 0; ; ++i)
	{
		char c = format[i];

		if(c == 0)
			break;

		if(c == '%')
		{
			switch(format[++i])
			{
			case 'd':
				if((PRINTF_BUF_LEN - bc - 1) < 12)
				{
					// Not enough buffer space
					buf[bc] = 0;

					__asm__ __volatile__("movl %0, %%ebx\n"
										 "movl %1, %%eax"
										 : : "r" (buf), "i" (FUNC_CODE_PRINT));
					__asm__ __volatile__("int $0x22");

					bc = 0;

					itoa(va_arg(va, int), &buf[bc], 10);

					while(buf[++bc] != 0);
				}
				else
				{
					itoa(va_arg(va, int), &buf[bc], 10);

					while(buf[++bc] != 0);
				}

				break;

			case 'c':
				if((PRINTF_BUF_LEN - bc - 1) < 1)
				{
					// Not enough buffer space
					buf[bc] = 0;

					__asm__ __volatile__("movl %0, %%ebx\n"
										 "movl %1, %%eax"
										 : : "r" (buf), "i" (FUNC_CODE_PRINT));
					__asm__ __volatile__("int $0x22");

					bc = 0;

					buf[bc++] = (char)va_arg(va, int);
				}
				else
				{
					buf[bc++] = (char)va_arg(va, int);
				}

				break;

			case 'x':
				if((PRINTF_BUF_LEN - bc - 1) < 10)
				{
					// Not enough buffer space
					buf[bc] = 0;

					__asm__ __volatile__("movl %0, %%ebx\n"
										 "movl %1, %%eax"
										 : : "r" (buf), "i" (FUNC_CODE_PRINT));
					__asm__ __volatile__("int $0x22");

					bc = 0;

					itoa(va_arg(va, int), &buf[bc], 16);

					while(buf[++bc] != 0);
				}
				else
				{
					itoa(va_arg(va, int), &buf[bc], 16);

					while(buf[++bc] != 0);
				}

				break;
			}
		}
		else
		{
			buf[bc] = c;

			if(++bc == (PRINTF_BUF_LEN - 1))
			{
				buf[bc] = 0;

				__asm__ __volatile__("movl %0, %%ebx\n"
									 "movl %1, %%eax"
									 : : "r" (buf), "i" (FUNC_CODE_PRINT));
				__asm__ __volatile__("int $0x22");

				bc = 0;
			}
		}
	}

	va_end(va);

	if(bc > 0)
	{
		buf[bc] = 0;

		__asm__ __volatile__("movl %0, %%ebx\n"
							 "movl %1, %%eax"
							 : : "r" (buf), "i" (FUNC_CODE_PRINT));
		__asm__ __volatile__("int $0x22");
	}
}