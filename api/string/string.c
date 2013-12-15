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

int strcmp(const char *str1, const char *str2)
{
	for(uint32_t i = 0; ; ++i)
	{
		if(str1[i] != str2[i])
			return (int)i + 1;

		if(str1[i] == 0)
			return 0;
	}
}

char *strtok(char *str, const char *delimiters, char **context)
{
	bool cont = FALSE;
	if(str == NULL)
	{
		str = *context;
		cont = TRUE;
	}

	for(uint32_t i = 0; ; ++i)
	{
		char c = str[i];

		if(c == 0)
		{
			if(cont)
			{
				if(*(char *)(*context - 1) == 0)
				{
					*context = &str[i];
					return str;
				}
			}

			break;
		}

		for(uint32_t j = 0; ; ++j)
		{
			char d = delimiters[j];

			if(d == 0)
				break;

			if(c == d)
			{
				str[i] = 0;
				*context = &str[i + 1];
				return str;
			}
		}
	}

	return NULL;
}
