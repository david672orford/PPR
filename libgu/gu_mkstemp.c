/*
** mouse:~ppr/src/libgu/gu_mkstemp.c
** Copyright 1995--2002, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appears in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is"
** without express or implied warranty.
**
** Last modified 18 January 2002.
*/

#include "before_system.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include "gu.h"

/*
** This is a replacement for a BSD function for making temporary
** files while preventing 'accidents' caused by symbolic links.
*/
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
	snprintf(template + XXXXXX_pos, 7, "%.6u", number);	/* room for 6 and a NULL */
	number++;
    	} while((fd = open(template, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR)) == -1 && errno == EEXIST && count < 1000000);

    return fd;
    }

/* end of file */
