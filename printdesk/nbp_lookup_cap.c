/*
** mouse:~ppr/src/printdesk/nbp_lookup_cap.c
** Copyright 1995--1999, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is"
** without express or implied warranty.
**
** Last modified 20 January 1999.
*/

/*
** An interface to nbp_lookup() which is used by atchooser
** and the CGI scripts in "../www".
**
** This source code file is used to produce an nbp_lookup
** that works with CAP.
*/

#include <stdio.h>

int main(int argc, char *argv[])
	{
	fprintf(stderr, "nbp_lookup: CAP version doesn't work\n");
	return 1;
	}

/* end of file */

