/*
** mouse:~ppr/src/libppr/parse_qfname.c
** Copyright 1995--2001, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appears in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is"
** without express or implied warranty.
**
** Last modified 30 March 2001.
*/

#include "config.h"
#include <string.h>
#include "gu.h"
#include "global_defines.h"
#include "global_structs.h"

/*
** This function parses a PPR queue file name and extracts the components of
** the jobid.  This function would be much simpler with regular expressions.
** because it is so complicated and touchy, both pprd and pprdrv use it.
** Note that ppop doesn't use this function since it must support partial
** (wildcard) jobid specifications.
*/
#ifdef COLON_FILENAME_BUG		/* If this operating system doesn't allow colons */
#define COLON_STR "!"			/* in file names, substitute exclamation points. */
#define COLON_CHAR '!'
#else
#define COLON_STR ":"
#define COLON_CHAR ':'
#endif
int parse_qfname(char *buffer, const char **destnode, const char **destname, short int *id, short int *subid, const char **homenode)
	{
	char *t_destnode, *t_destname, *t_homenode;
	int t_id, t_subid;
	char *ptr;
	int len;

	/* Find the extent of the destination node name. */
	t_destnode = buffer;
	len = strcspn(t_destnode, COLON_STR);
	if(t_destnode[len] != COLON_CHAR)
		return -1;
	t_destnode[len] = '\0';

	/* Find the extent of the destination queue name by getting
	   the length of part before dash followed by digit.
	   */
	{
	int c;
	t_destname = &t_destnode[len+1];
	for(len=0; TRUE; len++)
		{
		len += strcspn(&t_destname[len], "-");

		if(t_destname[len] == '\0')		/* check for end of string */
			break;

		if((c = t_destname[len+1+strspn(&t_destname[len+1], "0123456789")]) == '.' || c == '(' || c=='\0')
			break;
		}
	if(t_destname[len] != '-')
		return -2;
	t_destname[len] = '\0';				/* terminate destination name */
	}

	/* Scan for id number, subid number, and home node name. */
	ptr = &t_destname[len+1];
	if(gu_sscanf(ptr, "%d.%d", &t_id, &t_subid) != 2
			|| t_id < 0 || t_subid < 0)
		return -3;

	/* Find the home node name. */
	t_homenode = &ptr[strspn(ptr, "0123456789.")];
	if(*(t_homenode++) != '(' || (len = strlen(t_homenode)) < 2 || t_homenode[--len] != ')')
		return -1;
	t_homenode[len] = '\0';

	*destnode = t_destnode;
	*destname = t_destname;
	*id = t_id;
	*subid = t_subid;
	*homenode = t_homenode;
	return 0;
	}

/* end of file */
