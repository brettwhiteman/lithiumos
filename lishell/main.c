#include <stdio.h>
#include <string.h>

int main(void)
{
	char str[] = "This,is,a,comma,separated,list,of,words";
	char *context = NULL;
	
	char *s = strtok(str, ",", &context);

	while(s)
	{
		printf("\n");
		printf(s);
		s = strtok(NULL, ",", &context);
	}

	return 0; // :D
}
