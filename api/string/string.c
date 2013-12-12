#include <string.h>

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
