#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include "gu.h"


int gu_snprintfcat(char *buffer, size_t max, const char *format, ...)
	{
	va_list va;
	size_t len = strlen(buffer);
	int ret;
	max -= len;
	buffer += len;
	va_start(va, format);
	ret = vsnprintf(buffer, max, format, va);
	va_end(va);
	return ret;
	}

int gu_mkstemp(char *template)
	{
	int XXXXXX_pos = (strlen(template) - 6);
	unsigned int number, count;
	int fd;

	if(XXXXXX_pos < 0 || strcmp(template + XXXXXX_pos, "XXXXXX") != 0)
		{
		errno = EINVAL;
		return -1;
		}

	count = 0;
	number = (unsigned int)time(NULL) + (unsigned int)getpid();

	do	{
		while(number >= 1000000)
			number -= 1000000;
		snprintf(template + XXXXXX_pos, 7, "%.6u", number);		/* room for 6 and a NULL */
		number++;
		} while((fd = open(template, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR)) == -1 && errno == EEXIST && count < 1000000);

	return fd;
	}

