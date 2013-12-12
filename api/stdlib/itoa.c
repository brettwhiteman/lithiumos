#include <stdlib.h>
#include <types.h>
#include <string.h>

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
