/*
** mouse:~ppr/src/ppr/ppr_old2new.c
** Copyright 1995--2000, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Last modified 29 December 2000.
*/

/*
** This module contains a routine called old_to_new() which converts old
** format DSC comments (pre-3.0) to new format (3.0) comments.
*/

#include "before_system.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"

#include "global_structs.h"
#include "ppr.h"
#include "ppr_exits.h"

/*
** A subroutine which is used when replacing earlier version comments
** with DSC 3.0 comments.  Sets line[] to "%%comment tail" and
** calls tokenize() to parse the resulting line.
*/
static void fixcomment(const char comment[], char *tail)
    {
    const char function[] = "fixcomment";
    char tempstr[256];
    int x;

    x = 0;
    while((tail[x] == ' ' || tail[x] == '\t') && tail[x] != '\0')
	x++;			/* eat whitespace leading to tail */

    if(snprintf(tempstr, sizeof(tempstr), "%s", &tail[x]) < 0)
    	fatal(PPREXIT_OTHERERR, "%s(): assertion failed in line %d", function, __LINE__);

    if((line_len = snprintf(line, sizeof(line), "%%%%%s %s", comment, tempstr)) < 0)
     	fatal(PPREXIT_OTHERERR, "%s(): assertion failed in line %d", function, __LINE__);

    tokenize();			/* re-tokenize */
    } /* end of fixcomment() */

/*
** This function is like fixcomment(), but it takes its instructions in
** printf format.
*/
#ifdef __GNUC__
static void fixcommentf(const char format[], ...) __attribute__ (( format (printf, 1, 2) )) ;
#endif
static void fixcommentf(const char format[], ...)
    {
    const char function[] = "fixcommentf";
    va_list va;
    va_start(va, format);
    if((line_len = vsnprintf(line, sizeof(line), format, va)) < 0)
    	fatal(PPREXIT_OTHERERR, "%s(): assertion failed in line %d", function, __LINE__);
    va_end(va);
    tokenize();
    } /* end of fixcommentf() */

/*
** This function translates pre-DSC 3.0 comments to the equivelent DSC 3.0
** comments.  This routine is for comments which may occur anywhere in the
** file.  Obsolete comments which apply to the specic sections are handled
** without the help of this routine because they often require complex
** handling in the resource code.
**
** This is called from getline_simplify().  It is necessary for tokenize()
** to be called before calling this function.  If this function alters
** the line it will call tokenize() again to update the tokens list.
*/
void old_DSC_to_new(void)
    {
    switch(tokens[0][2])
	{
	case 'B':
	    if(strcmp(tokens[0], "%%BeginBinary:") == 0)
		{
		if(!tokens[1])
		    {
		    warning(WARNING_SEVERE, _("\"%s\" line has no argument"), "%%BeginBinary:");
		    }
		else
		    {
		    fixcommentf("%%%%BeginData: %s Binary Bytes", tokens[1]);
		    return;
		    }
		}
	    if(strcmp(tokens[0], "%%BeginFile:") == 0)
		{
		if(!tokens[1])
		    {
		    warning(WARNING_SEVERE, _("\"%s\" line has no argument"), "%%BeginFile:");
		    nest_push(NEST_BADRES, "[file name missing]");
		    }
		else
		    {
		    fixcommentf("%%%%BeginResource: file %s", tokens[1]);
		    return;
		    }
		}
	    if(strcmp(tokens[0], "%%BeginFont:") == 0)
		{
		if(!tokens[1])
		    {
		    warning(WARNING_SEVERE, _("\"%s\" line has no argument"), "%%BeginFont:");
		    nest_push(NEST_BADRES, "[font name missing]");
		    }
		else
		    {
		    fixcommentf("%%%%BeginResource: font %s", tokens[1]);
		    return;
		    }
		}
	    if(strcmp(tokens[0], "%%BeginProcSet:") == 0)
		{
		if(!tokens[1])
		    {
		    warning(WARNING_SEVERE, _("\"%s\" line has no arguments"), "%%BeginProcSet:");
		    nest_push(NEST_BADRES, "[procset name missing]");
		    }
		else
		    {
		    fixcomment("BeginResource: procset", line+15);
		    }
		return;
		}
	    if(strcmp(tokens[0], "%%BeginDocumentSetup") == 0 && !tokens[1])
		{
		warning(WARNING_PEEVE, _("\"%s\" should be \"%s\""), "%%BeginDocumentSetup", "%%BeginSetup");
		fixcommentf("%s", "%%BeginSetup");
		return;
		}
	    if(strcmp(tokens[0], "%%BeginPaperSize:") == 0)
		{
		if(qentry.attr.DSClevel >= 3.0)
		    warning(WARNING_PEEVE, _("\"%s\" is discontinued in DSC versions >= %s"), "%%BeginPaperSize:", "3.0");
		fixcomment("BeginFeature: *PageSize", line+17);
		return;
		}
	    return;

	case 'E':
	    if(strcmp(tokens[0], "%%EndBinary") == 0 && !tokens[1])
		{		/* convert to new style */
		fixcommentf("%s", "%%EndData");
		return;
		}

	    /* Fix some errors and fall thru. */
	    if(strcmp(tokens[0], "%%EndProcSet:") == 0)
	    	{
		warning(WARNING_PEEVE, _("\"%s\" does not take arguments"), "%%EndProcSet");
		fixcommentf("%s", "%%EndProcSet");
	    	}
	    else if(strcmp(tokens[0], "%%EndProcset") == 0)
	        {
		warning(WARNING_PEEVE, _("\"%s\" should be \"%s\""), "%%EndProcset", "%%EndProcSet");
		fixcommentf("%s", "%%EndProcSet");
	        }

	    if(strcmp(tokens[0], "%%EndFile") == 0
		|| strcmp(tokens[0], "%%EndFont") == 0
		|| strcmp(tokens[0], "%%EndProcSet") == 0)
		{
		/* If the opening comment was ok and was converted to the new format, convert
		   the closing comment too. */
		if(nest_inermost_type() != NEST_BADRES)
		    {
		    fixcommentf("%s", "%%EndResource");
		    }
		else	/* otherwise, pop the bad resource from the stack */
		    {
		    nest_pop(NEST_BADRES);
		    }
		return;
		}
	    if(strcmp(tokens[0], "%%EndDocumentSetup") == 0 && !tokens[1])
		{
		warning(WARNING_PEEVE, "\"%%%%EndDocumentSetup\" should be \"%%%%EndSetup\"");
		fixcommentf("%s", "%%EndSetup");
		return;
		}
	    if(strcmp(tokens[0], "%%ExecuteFile:") == 0)
	    	{
		if(qentry.attr.DSClevel >= 3.0)
		    warning(WARNING_PEEVE, "\"%%%%ExecuteFile:\" should be \"%%%%IncludeDocument:\" in DSC >= 3.0");
	    	fixcomment("IncludeDocument:", line+14);
		return;
	    	}
	    if(strcmp(tokens[0], "%%EndPaperSize") == 0 && !tokens[1])
	    	{
		fixcommentf("%s", "%%EndFeature");
		return;
	    	}
	    return;

	case 'F':
	    if(strcmp(tokens[0], "%%Feature:") == 0)
		{
		if(qentry.attr.DSClevel >= 3.0)
		    warning(WARNING_PEEVE, "\"%%%%Feature:\" should be \"%%%%IncludeFeature:\" in DSC >= 3.0");
		fixcomment("IncludeFeature:", line+10);
		return;
		}
	    return;

	case 'I':
	    if(strcmp(tokens[0], "%%IncludeFont:") == 0)
		{
		if(!tokens[1])
		    {
		    warning(WARNING_SEVERE, _("\"%s\" line has no argument"), "%%IncludeFont:");
		    }
		else
		    {
		    fixcommentf("%%%%IncludeResource: font %s", tokens[1]);
		    return;
		    }
		}
	    if(strcmp(tokens[0], "%%IncludeFile:") == 0)
		{
		if(!tokens[1])
		    {
		    warning(WARNING_SEVERE, _("\"%s\" line has no argument"), "%%IncludeFile:");
		    }
		else
		    {
		    fixcommentf("%%%%IncludeResource: file %s", tokens[1]);
		    return;
		    }
		}
	    if(strcmp(tokens[0], "%%IncludeProcSet:") == 0)
		{
		fixcomment("IncludeResource: procset", line+17);
		return;         /* must allow version and revision */
		}
	    return;
	default:
	    return;
	}
    } /* end of old_to_new() */

/* end of file */

