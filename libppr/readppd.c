/*
** mouse:~ppr/src/libppr/readppd.c
** Copyright 1995--2000, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is" without
** express or implied warranty.
**
** Last modified 14 December 2000.
*/

/*
** This module contains functions for opening and reading lines from PPD
** files.  Includes are handled automatically.
*/

#include "before_system.h"
#include <stdio.h>
#include <string.h>
#include "gu.h"
#include "global_defines.h"
#include "util_exits.h"

static int nest;			/* current PPD nesting level */
static FILE *f[MAX_PPD_NEST];		/* the list of open PPD files */
static char *fname[MAX_PPD_NEST];
static char *line = (char*)NULL;
static FILE *saved_errors;

/*
** Open the indicated PPD file.  If we can't open it, print an error
** message and return an appropriate exit code.
**
** This routine is called from ppd_open() and from ppd_readline()
** whenever an "*Include:" line is encountered.
*/
static int _ppd_open(const char *name)
    {
    if( ++nest >= MAX_PPD_NEST )		/* are we too deep? */
	{
	fprintf(saved_errors, "PPD files nested too deep:\n");
	fprintf(saved_errors, "\t\"%s\" included by:\n", name);
	for(nest--; nest >= 0; nest--)
	    {
	    fclose(f[nest]);
	    fprintf(saved_errors, "\t\"%s\"%s\n", fname[nest], nest ? " included by:" : "");
	    gu_free(fname[nest]);
	    }
	return EXIT_BADDEST;
	}

    /*
    ** If the PPD file name begins with a slash, use it as is,
    ** otherwise, prepend PPDDIR to it unless it is an included
    ** file in which case we prepend the directory of the
    ** including file.
    */
    if(name[0] == '/')
	{
	fname[nest] = gu_strdup(name);
	}
    else
	{
	if(nest == 0)
	    {
	    fname[nest] = (char*)gu_alloc(sizeof(PPDDIR) + 1 + strlen(name) + 1, sizeof(char));
    	    ppr_fnamef(fname[nest], "%s/%s", PPDDIR, name);
    	    }
	else
	    {
	    char *dirend;
	    int dirlen, buflen;

	    /* Get the offset of the last "/" in the previous path.
	       This should never fail.  If it does it is an
	       internal error. */
	    if(!(dirend = strrchr(fname[nest-1], '/')))
	    	{
		fprintf(saved_errors, "readppd.c: _ppd_open(): internal error\n");
	    	return EXIT_INTERNAL;
	    	}

	    /* Figure out how long the dirctory portion is and allocate
	       enough space to hold the whole thing. */
	    dirlen = (dirend - fname[nest-1]);
	    buflen = (dirlen + 1 + strlen(name) + 1);
	    fname[nest] = (char*)gu_alloc(buflen, sizeof(char));

	    /* Build the new name in the newly allocated space. */
	    snprintf(fname[nest], buflen, "%.*s/%s", dirlen, fname[nest-1], name);
	    }
	}

    /* Open the PPD file for reading. */
    if((f[nest] = fopen(fname[nest], "r")) == (FILE*)NULL )
    	{
	fprintf(saved_errors, "PPD file \"%s\" does not exist.\n", fname[nest]);
	gu_free(fname[nest--]);
	for( ; nest >= 0; nest--)
	    {
	    fclose(f[nest]);
	    fprintf(saved_errors, "\tincluded by: \"%s\"\n", fname[nest]);
	    gu_free(fname[nest]);
	    }
    	return EXIT_BADDEST;
    	}

    return EXIT_OK;
    } /* end of ppd_open() */

/*
** Open the indicated PPD file.  If we can't open it, print an error
** message and return an appropriate exit code.
*/
int ppd_open(const char *name, FILE *errors)
    {
    int retval;

    /*
    ** These functions use static storage.  Only one instance
    ** is allowed at a time.
    */
    if(line != (char*)NULL)
    	{
    	fprintf(errors, "ppd_open(): already open\n");
    	return EXIT_INTERNAL;
    	}

    saved_errors = errors;
    nest = -1;

    if((retval = _ppd_open(name)) == EXIT_OK)
	line = (char*)gu_alloc(MAX_PPD_LINE+2, sizeof(char));

    return retval;
    } /* end of ppd_open() */

/*
** Read the next line from the PPD file.  If we have reached the end
** of the file, return (char*)NULL.
**
** Comment lines are skipt and include files are transparently followed.
*/
char *ppd_readline(void)
    {
    if( line == (char*)NULL )
    	{	/* don't print to saved_errors, it might not be good! */
    	fprintf(stderr, "ppd_readline(): no file open\n");
    	return (char*)NULL;
    	}

    while(nest >= 0)
	{
	while( fgets(line, MAX_PPD_LINE+2, f[nest]) == (char*)NULL )
	    {
	    fclose(f[nest]);
	    gu_free(fname[nest]);	/* free the stored file name */
	    if(--nest < 0)		/* if we just closed the last file, */
	    	{
		gu_free(line);		/* free the line buffer */
		line = (char*)NULL;	/* leave a sign that there is no file open */
	    	return (char*)NULL;	/* and report end of file */
	    	}
	    }

	/* If this is a comment line, skip it. */
	if( strncmp(line, "*%", 2) == 0 )
	    continue;

	/* If this is an "*Include:" line, open a new file. */
	if( strncmp(line, "*Include:", 9) == 0 )
	    {
	    char *ptr;
	    int ret;

	    ptr = &line[9];
	    ptr += strspn(ptr, " \t\"");		/* find name start */
	    ptr[strcspn(ptr,"\"")] = '\0';		/* terminate name */

	    if( (ret=_ppd_open(ptr)) )
		{
		gu_free(line);
		line = (char*)NULL;
	    	return (char*)NULL;
		}

	    continue;
	    }

	return line;
	}

    return (char*)NULL;
    } /* end of ppd_readline() */

/* end of file */

