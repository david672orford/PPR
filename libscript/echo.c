#include <stdio.h>
#include <string.h>
int main(int argc, char *argv[])
	{
	int opt_n = 0;
	int i = 1;
	int count = 0;

	if(i < argc && strcmp(argv[i], "-n") == 0)
		{
		opt_n = 1;
		i++;
		}
		
	for( ; i < argc; i++)
		{
		if(count++ > 0)
			printf(" ");
		printf("%s", argv[i]);
		}

	if(!opt_n)
		printf("\n");

	return 0;
	}

/* end of file */
