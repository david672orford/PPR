/*
** mouse:~ppr/src/interfaces/alert.c
** Copyright 1995--1999, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Last modified 24 June 1999.
*/

/*
** This small program can be used by shell script printer
** interfaces which can't call alert() directly.
*/

#include "before_system.h"
#include <unistd.h>
#include "gu.h"
#include "global_defines.h"

#include "util_exits.h"

int main(int argc, char *argv[])
	{
	int stamp;

	chdir(HOMEDIR);

	if(argc != 4)
		{
		fprintf(stderr,"%s: wrong number of parameters\n", argv[0]);
		return EXIT_SYNTAX;
		}

	if((stamp = gu_torf(argv[2])) == ANSWER_UNKNOWN)
		{
		fprintf(stderr, "%s: second parameter must be TRUE or FALSE\n", argv[0]);
		return EXIT_SYNTAX;
		}

	/* Call the library alert function to post the alert. */
	alert(argv[1], stamp, argv[3]);

	/* We assume it worked, exit. */
	return EXIT_OK;
	} /* end of main() */

/* end of file */
