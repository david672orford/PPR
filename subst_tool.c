/*
 * subst_tool.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
	{
	int linenum = 0;
	char line[256];
	char *p;

	while(fgets(line, sizeof(line), stdin))
		{
		for(p=line; *p; p++)
			{
			linenum++;
			if(*p != '@')
				{
				fputc(*p, stdout);
				}
			else
				{
				char *end = strchr(++p, '@');
				const char *value;
				if(!end)
					{
					fprintf(stderr, "%s: unmatched @ in line %d\n", argv[0], linenum);
					return 1;
					}
				*end = '\0';
				if(!(value = getenv(p)))	
					{
					fprintf(stderr, "%s: no %s in environment\n", argv[0], p);
					return 1;
					}
				fputs(value, stdout);
				p = end;
				}
			}
		}

	return 0;
	}
