/*
** mouse:~ppr/src/ppr/ppr_dscdoc.c
** Copyright 1995--2001, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Last modified 18 July 2001.
*/

/*
** This module contains Adobe Document Structuring Convention (DSC)
** comment interpretation routines.
*/

#include "before_system.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "global_structs.h"
#include "ppr.h"
#include "ppr_infile.h"
#include "ppr_exits.h"

/*
** Flags which indicate that we may look for trailer comments for
** the indicated items.
*/
#define ATEND_CLEAR 0		/* initial state */
#define ATEND_KEYWORD_SEEN 1	/* the comment in some form has been seen */
#define ATEND_DEFERED 2		/* a valid atend form of the comment has been seen */
#define ATEND_WAS_DEFERED 4	/* a no longer valid atend has been seen */
static int atend_Pages = ATEND_CLEAR;
static int atend_PageOrder = ATEND_CLEAR;
static int atend_DocumentFonts = ATEND_CLEAR;
static int atend_DocumentProcSets = ATEND_CLEAR;
static int atend_Orientation = ATEND_CLEAR;
static int atend_DocumentNeededResources = ATEND_CLEAR;
static int atend_DocumentSuppliedResources = ATEND_CLEAR;
static int atend_DocumentNeededFiles = ATEND_CLEAR;
static int atend_DocumentNeededFonts = ATEND_CLEAR;
static int atend_DocumentNeededProcSets = ATEND_CLEAR;
static int atend_DocumentSuppliedFiles = ATEND_CLEAR;
static int atend_DocumentSuppliedFonts = ATEND_CLEAR;
static int atend_DocumentSuppliedProcSets = ATEND_CLEAR;

/*
** Flags to indicated when things which can only appear
** in the header have been found.  These flags are
** set to TRUE to prevent subsequent instances from
** being used.
*/
static int found_For = FALSE;
static int found_Routing = FALSE;
static int found_Title = FALSE;
static int found_ProofMode = FALSE;
static int found_Creator = FALSE;
static int found_DocumentData = FALSE;

/* Flags for things that can be declared (atend). */
static int found_DocumentFonts = 0;
static int found_DocumentNeededResources = 0;
static int found_DocumentSuppliedResources = 0;
static int found_DocumentProcSets = 0;
static int found_DocumentNeededFiles = 0;
static int found_DocumentNeededFonts = 0;
static int found_DocumentNeededProcSets = 0;
static int found_DocumentSuppliedFiles = 0;
static int found_DocumentSuppliedFonts = 0;
static int found_DocumentSuppliedProcSets = 0;

/*
** This routine is called when at least one argument is
** required for a keyword.  It issues a warning and
** returns non-zero if the argument is missing.
*/
static int trap_noarg(const char *keyword, const char *argument)
    {
    if(!argument)
    	{
    	warning(WARNING_SEVERE, _("\"%s\" comment has no argument"), keyword);
	return -1;
    	}
    return 0;
    } /* end of trap_noarg() */

/*
** This function is like trap_noarg() above but is detects the
** absence of a list.  It is unclear from the DSC specification
** if empty lists are allowed.
*/
static int trap_nolist(const char *keyword, const char *argument)
    {
    if(argument == (char*)NULL)
    	return -1;
    return 0;
    } /* end of trap_nolist() */

/*
** This function is called to check for an argument of "(atend)".  If
** it is found and the trailer parameter is FALSE, the flag is set.
** If trailer is TRUE, a warning is issued.  If atend is present, the
** return value is non-zero whether or not a warning is waranted.
** trap_noarg() or trap_nolist() should be called before this function.
*/
static int handle_atend(const char *keyword, const char *argument, int argcount, int *atend_flag, int trailer)
    {
    int old_atend_flag = *atend_flag;
    *atend_flag |= ATEND_KEYWORD_SEEN;

    if(strcmp(argument, "atend") == 0 && argcount == 2)
    	{
	/* Atend has no meaning in the trailer section. */
	if(trailer)
	    {
	    warning(WARNING_SEVERE, _("Keyword \"%s\" declared (atend) in trailer"), keyword);
	    }
	else
	    {
	    /* Only the first instance is valid: */
	    if( ! (old_atend_flag & ATEND_KEYWORD_SEEN) )
		*atend_flag |= ATEND_DEFERED;
	    else
	    	*atend_flag |= ATEND_WAS_DEFERED;
	    }

    	return -1;
    	}
    /* Instances of this keyword in the header which come
       after an atend instance
       have been superseeded. */
    else if(! trailer && *atend_flag & ATEND_DEFERED)
	{
	return -1;
	}
    /* Instances in the trailer don't count unless
       the keyword was defered in the header. */
    else if(trailer && ! (*atend_flag & ATEND_DEFERED))
    	{
	/* If there is an old (atend) in the header, we
	   will ommit the warning. */
	if( ! (*atend_flag & ATEND_WAS_DEFERED) )
	    warning(WARNING_SEVERE, "Ignoring \"%s\" in trailer because no \"%s (atend)\" in header", keyword, keyword);
	return -1;
    	}
    return 0;
    } /* end of handle_atend() */

/*
** This function is called if atend is not allowed for the keyword in question.
** If the argument is atend, a warning is issued and the return value is
** non-zero.  If we are in the trailer, a different warning is issued.
**
** trap_noarg() or trap_nolist() should be called before this function.
*/
static int trap_atend(const char *keyword, const char *argument, int argcount, int trailer)
    {
    if(strcmp(argument, "atend") == 0 && argcount == 2)
    	{
	if(trailer)
	    warning(WARNING_SEVERE, _("Keyword \"%s\" can't take (atend), especially not in trailer"), keyword);
	else
    	    warning(WARNING_SEVERE, _("Keyword \"%s\" can't take (atend)"), keyword);
	return -1;
    	}
    if(trailer)
    	{
    	warning(WARNING_SEVERE, "Keyword \"%s\" is not allowed in trailer, ignoring", keyword);
    	return -1;
    	}
    return 0;
    } /* end of trap_atend() */

/*
** This function is used to achieve consistent wording.
*/
static void warning_invalid_arg(const char *keyword, const char *argument)
    {
    warning(WARNING_SEVERE, "Invalid \"%s\" argument \"%s\"", keyword, argument);
    } /* end of warning_invalid_arg() */

/*
** Return true if the resource in the comment should be
** processed.  If we are in the trailer section and a previous
** version of this line has been seen (one with a different
** value of dsc_comment_number) then clear all resources references
** of the indicated type for the indicated type of resource.
**
** All continuation lines have the same dsc_comment_number as
** the main line.
*/
static int handle_superseded(int trailer, int *found_flag, int reftype, const char *restype)
    {
    if(trailer || ! *found_flag || dsc_comment_number == *found_flag)
	{
	if(trailer && *found_flag && *found_flag != dsc_comment_number)
	    resource_clear(reftype, restype);
	*found_flag = dsc_comment_number;
	return TRUE;
	}
    return FALSE;
    } /* end of handle_superseded() */

/*
** This is called from ppr_infile.c and by header_trailer() below.
** It checks to see if the header line has been seen yet and if
** it hasn't, it makes a copy of the value and stores it.
*/
void do_dsc_Routing(const char *routingstr)
    {
    if(read_Routing && ! found_Routing)
    	{
	qentry.Routing = gu_strdup(routingstr);
	found_Routing = TRUE;
    	}
    } /* end of do_dsc_Routing */

/*
** This is called from ppr_infile.c and by header_trailer() below.  It checks 
** to see if the "%%For:" header line has been seen yet and if it hasn't, it
** makes a copy of the value and stores it.
*/
void do_dsc_For(const char *forstr)
    {
    if(read_For && ! found_For)
    	{
	int x;
	char *ptr = gu_strdup(forstr);

	/*
	** Turn unprintable characters into periods.  This
	** code is here because some people somehow manage
	** to get deletes into their Macintosh computer names!
	*/
	for(x=0; ptr[x]; x++)
	    {
	    if(!isprint(ptr[x]))
		ptr[x] = '.';
	    }

	qentry.For = ptr;
	found_For = TRUE;
    	}
    } /* end of do_dsc_For */

/*
** Handle header and trailer comments.
**
** Return non zero if line should NOT be copied to the
** comments file.
**
** We will call old_to_new_headertrailer() to convert pre DSC 3.0
** comments to DSC 3.0.
**
** tokenize() should be called before this routine is called.
**
** The argument "trailer" should be true when this routine is
** called to handle the document trailer.
*/
static int header_trailer(gu_boolean trailer)
    {
    if(! tokens[0])
    	fatal(PPREXIT_OTHERERR, "header_trailer() line %d: assertion failed", __LINE__);

    switch(line[2])
	{
	case 'P':
	    if(strcmp(tokens[0], "%%Pages:") == 0)
		{
		/* Warning for "%%Pages: (atend) 1" which RBIIp 703
		   suggests is not allowed: */
		if(tokens_count == 3 && strcmp(tokens[1], "atend") == 0)
		    warning(WARNING_SEVERE, "Second argument to \"%%%%Pages:\" is invalid with (atend), ignoring it");

		/* As part of "%%Pages: (atend) 1" handling, we don't let
		   handle_atend() consider no more than two tokens: */
		if(trap_noarg(tokens[0], tokens[1])
			|| handle_atend(tokens[0], tokens[1], tokens_count > 2 ? 2 : tokens_count, &atend_Pages, trailer))
		    return -1;

		/* If this is the trailer or the first "%%Pages:"
		   comment in the header, heed it.
		   */
		if(trailer || qentry.attr.pages == -1)
		    {
		    /* Read the page count and possible a 2.x PageOrder: */
		    int cnt = sscanf(line, "%%%%Pages: %d %d",
				    &qentry.attr.pages,
				    &qentry.attr.pageorder);

		    /* If no numberic arguments: */
		    if(cnt == 0)
			warning(WARNING_SEVERE, "Comment \"%s\" is invalid", line);

		    /* Deprecated in DSC 3.0 (RBIIp 703): */
		    if(cnt == 2 && qentry.attr.DSClevel >= 3.0)
		    	warning(WARNING_PEEVE, "2nd argument to \"%%%%Pages:\" is deprecated in DSC >= 3.0");
		    }
		return -1;      /* use only first */
		}
	    if(strcmp(tokens[0], "%%PageOrder:") == 0)
		{
		if(trap_noarg(tokens[0], tokens[1]) || handle_atend(tokens[0], tokens[1], tokens_count, &atend_PageOrder, trailer))
		    return -1;

		if(strcmp(tokens[1], "Ascend") == 0)
		    qentry.attr.pageorder = PAGEORDER_ASCEND;
		else if(strcmp(tokens[1], "Descend") == 0)
		    qentry.attr.pageorder = PAGEORDER_DESCEND;
		else if(strcmp(tokens[1], "Special") == 0)
		    qentry.attr.pageorder = PAGEORDER_SPECIAL;
		else if(strcmp(tokens[1], "Ascending") == 0)
		    {
		    warning(WARNING_PEEVE, "\"%%%%PageOrder: Ascending\" should be \"%%%%PageOrder: Ascend\"");
		    qentry.attr.pageorder = PAGEORDER_ASCEND;
		    }
		else if(strcmp(tokens[1], "Descending") == 0)
		    {
		    warning(WARNING_PEEVE, "\"%%%%PageOrder: Descending\" should be \"%%%%PageOrder: Descend\"");
		    qentry.attr.pageorder = PAGEORDER_DESCEND;
		    }
		else
		    warning_invalid_arg(tokens[0], tokens[1]);
		return -1;
		}
	    if(strncmp(line, "%%Page", 5) == 0)	/* detect things like */
		{				/* %%PageBoundingBox: */
		if(trailer)
		    warning(WARNING_PEEVE, "Page level keyword \"%s\" in trailer", line);
		else
		    warning(WARNING_PEEVE, "Page level keyword \"%s\" in header comments", line);
		return 0;                       /* but leave it in */
		}
	    if(strcmp(tokens[0], "%%ProofMode:") == 0)
	    	{
		if(trap_noarg(tokens[0], tokens[1]) || trap_atend(tokens[0], tokens[1], tokens_count, trailer))
		    return -1;

		/* If ProofMode not already set */
		if(found_ProofMode == FALSE)
		    {
		    if(tokens[1] == (char*)NULL)			/* parameter missing, */
		    	qentry.attr.proofmode = PROOFMODE_SUBSTITUTE;	/* (RBIIp 775) */
		    else if(strcmp(tokens[1], "TrustMe") == 0)
		        qentry.attr.proofmode = PROOFMODE_TRUSTME;
		    else if(strcmp(tokens[1], "Substitute") == 0)
		        qentry.attr.proofmode = PROOFMODE_SUBSTITUTE;
		    else if(strcmp(tokens[1], "NotifyMe") == 0)
		        qentry.attr.proofmode = PROOFMODE_NOTIFYME;
		    else
			warning_invalid_arg(tokens[0], tokens[1]);

		    found_ProofMode = TRUE;
		    }
		return -1;
		}
	    break;

	case 'F':
	    if(strcmp(tokens[0], "%%For:") == 0)
		{
		char *ptr;

		if(trap_noarg(tokens[0], tokens[1]) || trap_atend(tokens[0], tokens[1], tokens_count, trailer))
		    return -1;

		ptr = line + 6;
		ptr += strspn(ptr, " \t");

		/* If it is a PS string, use the first token. */
		if(*ptr == '(')
		    ptr = tokens[1];

		do_dsc_For(ptr);

		return -1;			/* Don't keep the line. */
		}
	    break;
	case 'L':
	    if(strcmp(tokens[0], "%%LanguageLevel:") == 0)
		{
		int x;

		if(trap_noarg(tokens[0], tokens[1]) || trap_atend(tokens[0], tokens[1], tokens_count, trailer))
		    return -1;

		if((x = atoi(tokens[1])) >= 1)
		    {
		    /* Record it for easy future reference: */
		    qentry.attr.langlevel = x;
		    /* Let it get into the comments file: */
		    return 0;
		    }
		else
		    {
		    warning_invalid_arg(tokens[0], tokens[1]);
		    }
		return -1;
		}
	    break;
	case 'T':
	    if(strcmp(tokens[0], "%%Title:") == 0)
		{
		if(trap_noarg(tokens[0], tokens[1]) || trap_atend(tokens[0], tokens[1], tokens_count, trailer))
		    return -1;

		if( ! found_Title )
		    {
		    char *ptr = line + 8;
		    ptr += strspn(ptr, " \t");
		    if(*ptr=='(')
			qentry.Title = gu_strdup(tokens[1]);
		    else		/* ^ NULL ptr safe */
			qentry.Title = gu_strdup(ptr);
		    found_Title = TRUE;
		    }
		return -1;
		}
	    break;
	case 'C':
	    if(strcmp(tokens[0], "%%Creator:") == 0)
		{
		if(trap_noarg(tokens[0], tokens[1]) || trap_atend(tokens[0], tokens[1], tokens_count, trailer))
		    return -1;

		if( ! found_Creator )
		    {
		    char *ptr = line + 10;
		    ptr += strspn(ptr, " \t");
		    if(*ptr == '(')		/* if PostScript string, */
			qentry.Creator = gu_strdup(tokens[1]); /* use 1st token */
		    else			/* if a bare string, */
			qentry.Creator = gu_strdup(ptr); /* use rest of line */
		    found_Creator = TRUE;
		    }
		return -1;
		}
	    if(strcmp(tokens[0], "%"PPR_DSC_PREFIX"AuthCode:") == 0)
		{
		if(trap_noarg(tokens[0], tokens[1]) || trap_atend(tokens[0], tokens[1], tokens_count, trailer))
		    return -1;

		/* If this is the first AuthCode line, then save the code. */
		if(AuthCode == (char*)NULL)
		    AuthCode = gu_strdup(tokens[1]);

		return -1;		/* swallow line */
		}
	    break;
	case 'E':
	    if(strcmp(tokens[0], "%%Extensions:") == 0)
		{
		int x;

		if(trap_nolist(tokens[0], tokens[1]) || trap_atend(tokens[0], tokens[1], tokens_count, trailer))
		    return -1;

		for(x=1; tokens[x] != (char*)NULL; x++)
		    {
		    if(strcmp(tokens[x], "DPS") == 0)
			qentry.attr.extensions |= EXTENSION_DPS;
		    else if(strcmp(tokens[x], "CMYK") == 0)
			qentry.attr.extensions |= EXTENSION_CMYK;
		    else if(strcmp(tokens[x], "Composite") == 0)
			qentry.attr.extensions |= EXTENSION_Composite;
		    else if(strcmp(tokens[x],"FileSystem") == 0)
			qentry.attr.extensions |= EXTENSION_FileSystem;
		    else
			warning(WARNING_SEVERE, "Language extension \"%s\" is unrecognized", tokens[x]);
		    }
		fprintf(comments,"%s\n",line);  /* this is faster than */
		return -1;                      /* returning zero */
		}
	    break;
	case 'R':
	    if(strcmp(tokens[0], "%%Requirements:") == 0)
		{
		int x;

		if(trap_nolist(tokens[0], tokens[1]) || trap_atend(tokens[0], tokens[1], tokens_count, trailer))
		    return -1;

		for(x=1; tokens[x] != (char*)NULL; x++)
		    requirement(REQ_DOC, tokens[x]);

		return -1;
		}
	    if(strcmp(tokens[0], "%%Routing:") == 0)
		{
		char *ptr;

		if(trap_noarg(tokens[0], tokens[1]) || trap_atend(tokens[0], tokens[1], tokens_count, trailer))
		    return -1;

		ptr = line + 10;
		ptr += strspn(ptr, " \t");

		if(*ptr == '(')			/* if PostScript string, */
		    do_dsc_Routing(tokens[1]);	/* use 1st token */
		else				/* if bare string, */
		    do_dsc_Routing(ptr);	/* use rest of line */

		return -1;
		}
	    break;

	case 'D':
	    if(strcmp(tokens[0], "%%DocumentMedia:") == 0)
		{
		if(trap_nolist(tokens[0], tokens[1]) || trap_atend(tokens[0], tokens[1], tokens_count, trailer))
		    return -1;

		media(MREF_DOC, 1);
		return -1;
		}
	    if(strcmp(tokens[0], "%%DocumentFonts:") == 0)
		{
		int x;

		if(trap_nolist(tokens[0], tokens[1]) || handle_atend(tokens[0], tokens[1], tokens_count, &atend_DocumentFonts, trailer))
		    return -1;

		/*
		** Feed each of the font names to the resource handler,
		** telling the resource handler that it is unclear
		** whether the font is required or supplied.
		*/
		if(handle_superseded(trailer, &found_DocumentFonts, REREF_UNCLEAR, "font"))
		    for(x=1; tokens[x] != (char*)NULL; x += resource(REREF_UNCLEAR, "font", x)) ;

		return -1;	/* <-- pprdrv will reconstruct font comments */
		}
	    if(strcmp(tokens[0], "%%DocumentProcSets:") == 0)
		{
		int x;

		if(trap_nolist(tokens[0], tokens[1]) || handle_atend(tokens[0], tokens[1], tokens_count, &atend_DocumentProcSets, trailer))
		    return -1;

		if(handle_superseded(trailer, &found_DocumentProcSets, REREF_UNCLEAR, "procset"))
		    for(x=1; tokens[x]!=(char*)NULL; x += resource(REREF_UNCLEAR, "procset", x)) ;

		return -1;
		}
	    if(strcmp(tokens[0], "%%DocumentNeededResources:") == 0)
		{
		int x;

		if(trap_nolist(tokens[0], tokens[1]) || handle_atend(tokens[0], tokens[1], tokens_count, &atend_DocumentNeededResources, trailer))
		    return -1;

		if(handle_superseded(trailer, &found_DocumentNeededResources, REREF_NEEDED, (char*)NULL))
		    for(x=2; tokens[x] != (char*)NULL; x += resource(REREF_NEEDED, tokens[1], x)) ;

		return -1;
		}
	    if(strcmp(tokens[0], "%%DocumentNeededFiles:") == 0)
		{
		int x;

		if(trap_nolist(tokens[0], tokens[1]) || handle_atend(tokens[0], tokens[1], tokens_count, &atend_DocumentNeededFiles, trailer))
		    return -1;

		if(handle_superseded(trailer, &found_DocumentNeededFiles, REREF_NEEDED, "file"))
		    for(x=1; tokens[x] != (char*)NULL; x += resource(REREF_NEEDED, "file", x)) ;

		return -1;
		}
	    if(strcmp(tokens[0], "%%DocumentNeededFonts:") == 0)
		{
		int x;

		if(trap_nolist(tokens[0], tokens[1]) || handle_atend(tokens[0], tokens[1], tokens_count, &atend_DocumentNeededFonts, trailer))
		    return -1;

		if(handle_superseded(trailer, &found_DocumentNeededFonts, REREF_NEEDED, "font"))
		    for(x=1; tokens[x] != (char*)NULL; x += resource(REREF_NEEDED, "font", x)) ;

		return -1;
		}
	    if(strcmp(tokens[0], "%%DocumentNeededProcSets:") == 0)
		{
		int x;

		if(trap_nolist(tokens[0], tokens[1]) || handle_atend(tokens[0], tokens[1], tokens_count, &atend_DocumentNeededProcSets, trailer))
		    return -1;

		if(handle_superseded(trailer, &found_DocumentNeededProcSets, REREF_NEEDED, "procset"))
		    for(x=1; tokens[x] != (char*)NULL; x += resource(REREF_NEEDED, "procset", x)) ;

		return -1;
		}
	    if(strcmp(tokens[0], "%%DocumentSuppliedResources:") == 0)
		{
		int x;

		if(trap_nolist(tokens[0], tokens[1]) || handle_atend(tokens[0], tokens[1], tokens_count, &atend_DocumentSuppliedResources, trailer))
		    return -1;

		if(handle_superseded(trailer, &found_DocumentSuppliedResources, REREF_SUPPLIED, (char*)NULL))
		    for(x=2; tokens[x] != (char*)NULL; x += resource(REREF_SUPPLIED, tokens[1], x)) ;

		return -1;
		}
	    if(strcmp(tokens[0], "%%DocumentSuppliedFiles:") == 0)
		{
		int x;

		if(trap_nolist(tokens[0], tokens[1]) || handle_atend(tokens[0], tokens[1], tokens_count, &atend_DocumentSuppliedFiles, trailer))
		    return -1;

		if(handle_superseded(trailer, &found_DocumentSuppliedFiles, REREF_SUPPLIED, "file"))
		    for(x=1; tokens[x] != (char*)NULL; x += resource(REREF_SUPPLIED, "file", x)) ;

		return -1;
		}
	    if(strcmp(tokens[0], "%%DocumentSuppliedFonts:") == 0)
		{
		int x;

		if(trap_nolist(tokens[0], tokens[1]) || handle_atend(tokens[0], tokens[1], tokens_count, &atend_DocumentSuppliedFonts, trailer))
		    return -1;

		if(handle_superseded(trailer, &found_DocumentSuppliedFonts, REREF_SUPPLIED, "font"))
		    for(x=1; tokens[x] != (char*)NULL; x += resource(REREF_SUPPLIED, "font", x)) ;

		return -1;
		}
	    if(strcmp(tokens[0], "%%DocumentSuppliedProcSets:") == 0)
		{
		int x;

		if(trap_nolist(tokens[0], tokens[1]) || handle_atend(tokens[0], tokens[1], tokens_count, &atend_DocumentSuppliedProcSets, trailer))
		    return -1;

		if(handle_superseded(trailer, &found_DocumentSuppliedProcSets, REREF_SUPPLIED, "procset"))
		    for(x=1; tokens[x] != (char*)NULL; x += resource(REREF_SUPPLIED, "procset", x)) ;

		return -1;
		}
	    if(strcmp(tokens[0], "%%DocumentData:") == 0)
	    	{
		if(trap_nolist(tokens[0], tokens[1]) || trap_atend(tokens[0], tokens[1], tokens_count, trailer))
		    return -1;

		if( ! found_DocumentData )
		    {
		    if(strcmp(tokens[1], "Clean7Bit") == 0)
		    	qentry.attr.docdata = CODES_Clean7Bit;
		    else if(strcmp(tokens[1], "Clean8Bit") == 0)
		    	qentry.attr.docdata = CODES_Clean8Bit;
		    else if(strcmp(tokens[1], "Binary") == 0)
		    	qentry.attr.docdata = CODES_Binary;
		    else
			warning(WARNING_SEVERE, "Invalid \"%%%%DocumentData: %s\"", tokens[1]);

		    found_DocumentData = TRUE;
		    }

		return -1;
		}

	    /* Examine only the first part to catch
	       %%DocumentPaperSize:, %%DocumentPaperColor:, etc. */
	    if(strncmp(line, "%%DocumentPaper", 15) == 0)
	    	{
		warning(WARNING_SEVERE, "Obsolete \"%s\" comment ignored", line);
		return 0;
	    	}
	    break;

	case 'O':
	    if(strcmp(tokens[0], "%%Orientation:") == 0)
	    	{
		if(trap_noarg(tokens[0], tokens[1]) || handle_atend(tokens[0], tokens[1], tokens_count, &atend_Orientation, trailer))
		    return -1;

		/* If this is the first Orientation comment or this is the trailer, */
		if(trailer || qentry.attr.orientation == ORIENTATION_UNKNOWN )
		    {
		    /*
		    ** Interpret arguments "Portrait" and "Landscape" but object
		    ** to anything else except "atend" in the header.
		    */
		    if(strcmp("Portrait", tokens[1]) == 0)
			qentry.attr.orientation = ORIENTATION_PORTRAIT;
		    else if(strcmp("Landscape", tokens[1]) == 0)
			qentry.attr.orientation = ORIENTATION_LANDSCAPE;
		    else if(trailer || strcmp(tokens[1], "atend"))
			warning_invalid_arg(tokens[0], tokens[1]);
		    }

		return -1;	/* <-- pprdrv will generate new comment */
	    	}
	    break;

	} /* end of switch */

    /* Remove unrecognized (atend) comments, presuming that
       the matching comment will be in the trailer.  Since we
       copy both header and trailer comments into the -comments
       file and they all end up in the header comments section,
       we have no use for (atend) comments. */
    if(tokens_count == 2 && strcmp(tokens[1], "atend") == 0)
	{
	if(trailer)
	    warning(WARNING_SEVERE, "Keyword \"%s\" declared (atend) in trailer", tokens[0]);
	return -1;
	}

    return 0;       /* just copy it */
    } /* end of header_trailer() */

/*
** Handle page level comments.  Return zero if we want it copied into
** he -pages or -text file.  If we return -1 it is understood that we
** made our own use of it and don't want it copied to the output file.
*/
static int pagelevel(void)
    {
    if(! tokens[0])
    	fatal(PPREXIT_OTHERERR, "pagelevel() line %d: assertion failed", __LINE__);

    if(strncmp(line, "%%Page", 6) == 0)
	{
	switch(line[6])
	    {
	    case 'R':
		if(strcmp(tokens[0], "%%PageResources:") == 0)
		    {
		    int x;
		    for(x=2; tokens[x] != (char*)NULL; x += resource(REREF_PAGE, tokens[1], x)) ;
		    return -1;  /* don't copy to output */
		    }
		if(strcmp(tokens[0], "%%PageRequirements:") == 0)
		    {
		    int x;
		    for(x=1; tokens[x]!=(char*)NULL; x++)
			requirement(REQ_PAGE,tokens[x]);
		    return -1;
		    }
		break;
	    case 'F':
		if(strcmp(tokens[0], "%%PageFonts:") == 0)
		    {
		    int x;
		    for(x=1; tokens[x] != (char*)NULL; x += resource(REREF_PAGE, "font", x)) ;
		    return -1;
		    }
		if(strcmp(tokens[0], "%%PageFiles:") == 0)
		    {
		    int x;
		    for(x=1; tokens[x] != (char*)NULL; x += resource(REREF_PAGE, "file", x)) ;
		    return -1;
		    }
		break;
	    case 'M':
		if(strcmp(tokens[0], "%%PageMedia:") == 0)
		    {
		    media(MREF_PAGE, 1);
		    return -1;
		    }
		break;
	    }
	}
    else if(strncmp(line, "%%Paper", 7) == 0)
    	{
    	warning(WARNING_SEVERE, "Obsolete \"%s\" comment ignored", line);
    	}

    return 0;
    } /* end of pagelevel() */

/*
** This routine is called by read_prolog() whenever a "%%BeginFeature:"
** or "%%IncludeFeature:" line is seen.  This allows us to gather
** information about the characteristics of the desired medium, if
** feature code selects it, even if there is no "%%Media:" comment.
** It also enables us to implement certain non-existent printer
** features such as booklet mode in the spooler.
*/
static void feature_spy(void)
    {
    if(tokens[1] && tokens[2])
    	{
	switch(tokens[1][1])
	    {
	    case 'B':
	    	/* If "Booklet", set booklet mode true. */
		if(read_signature && strcmp(tokens[1], "*Booklet") == 0)
		    {
		    if(strcmp(tokens[2], "True")==0)
		    	qentry.N_Up.sigsheets = (-1);	/* emulate -s booklet */
		    }
		break;

	    case 'D':
	    	/* If it was a duplex feature and we are looking for them,
	    	   remember what it was. */
	    	if(read_duplex && strcmp(tokens[1], "*Duplex") == 0)
		    {
		    if(strcmp(tokens[2], "None") == 0)
		    	current_duplex = DUPLEX_NONE;
		    else if(strcmp(tokens[2], "DuplexNoTumble") == 0)
		    	current_duplex = DUPLEX_DUPLEX_NOTUMBLE;
		    else if(strcmp(tokens[2], "DuplexTumble") == 0)
		    	current_duplex = DUPLEX_DUPLEX_TUMBLE;
		    }
		break;

	    case 'M':
		/* Remeber various media characteristics in case
		   no %%Media: comment was provided. */
	    	if(strcmp(tokens[1], "*MediaColor") == 0)
	    	    {
	    	    ASCIIZ_to_padded(guess_media. colour, tokens[2], MAX_COLOURNAME);
	    	    }
		else if(strcmp(tokens[1], "*MediaType") == 0)
		    {
		    if(strcmp(tokens[2], "Plain") == 0)		/* PPD file might use "Plain" */
			ASCIIZ_to_padded(guess_media. type, "", MAX_TYPENAME);
		    else if(strcmp(tokens[2], "None") == 0)	/* PPD file might use "None" */
			ASCIIZ_to_padded(guess_media. type, "", MAX_TYPENAME);
		    else
		    	ASCIIZ_to_padded(guess_media.type,tokens[2],MAX_TYPENAME);
		    }
		else if(strcmp(tokens[1], "*MediaWeight")==0)
		    {
		    guess_media.weight=gu_getdouble(tokens[2]);
		    }
		break;

	    case 'N':
	    	if(read_nup && strcmp(tokens[1], "*pprN-Up") == 0)
		    {
		    int temp = atoi(tokens[2]);
		    if(temp >= 1 || temp <= 16)
		    	{
			qentry.N_Up.N = temp;
		    	}
		    }
		else if(read_nup && strcmp(tokens[1], "*pprN-UpBorders") == 0)
		    {
		    if(strcmp(tokens[2], "False") == 0)
		    	qentry.N_Up.borders = FALSE;
		    else if(strcmp(tokens[2], "True") == 0)
		    	qentry.N_Up.borders = TRUE;
		    }
	    	break;

	    case 'P':
		/*
		** For the media size, we will remember it
		** for use if there is no %%Media: comment.
		*/
		if(strcmp(tokens[1], "*PageSize") == 0 || strcmp(tokens[1],"*PageRegion") == 0)
		    {
		    double width, height;
		    gu_boolean envelope;
		    if(pagesize(tokens[2], NULL, &width, &height, &envelope) != -1)
			{
			#ifdef DEBUG_MEDIA
			printf("%s %dx%d, %s\n", tokens[2], width, height, envelope ? "True" : "False");
			#endif
			guess_media.width = width;
			guess_media.height = height;
			if(envelope)
			    ASCIIZ_to_padded(guess_media.type, "Envelope", MAX_TYPENAME);
			}
		    }
	    	break;

	    case 'S':
	    	/* If sigature, set the signature mode */
		if(read_signature && strcmp(tokens[1], "*Signature") == 0)
		    {
		    if(strcmp(tokens[2], "True") == 0)	/* If the option is */
		    	{				/* simply "True", */
		    	qentry.N_Up.sigsheets = 8;	/* enable 8 sheet (32 page) */
		    	}				/* signatures. */
		    else				/* otherwise */
		    	{				/* who knows? */
			if(strspn(tokens[2],"0123456789") == strlen(tokens[2]))
			    {
			    qentry.N_Up.sigsheets = ((atoi(tokens[2])+3) / 4);
			    }
			else
			    {

			    }
		    	}
		    }
		break;

	    default:
	    	break;
	    }
    	}
    } /* end of feature_spy() */

/*=========================================================================
** A routine for each section of a DSC conforming document.
=========================================================================*/

/*
** Read the header comments.
** The comments will be put into the comments file and into the Thing
** structures.  We must return with a line in the line buffer.
**
** Most of the comment work will be done by header_trailer().
*/
void read_header_comments(void)
    {
    outermost_start(OUTERMOST_HEADER_COMMENTS);

    getline_simplify_cache();                /* get 1st line, hopefully %!PS-... */

    /*
    ** Process the %! line.
    */
    if(strncmp(line, "%!PS-Adobe-", 11) == 0 && sscanf(line, "%%!PS-Adobe-%f", &qentry.attr.DSClevel) == 1)
	{
	if(qentry.attr.DSClevel <= 3.0)			/* if below or at */
	    {
	    fprintf(comments, "%%!PS-Adobe-3.0\n");	/* our version say ours */
	    }
	else						/* if higher, */
	    {
	    fprintf(comments, "%s\n", line);		/* say that version */
	    warning(WARNING_SEVERE, _("Document uses DSC version %.1f which is > 3.0"), qentry.attr.DSClevel);
	    }

	getline_simplify_cache();			/* and refill buffer */
	}
    else				/* If not flagged with */
	{				/* "%!PS-Adobe-x.xx", */
	fprintf(comments, "%%!\n");	/* make no version claims. */
	}

    /*
    ** Process all of the header lines.
    */
    for( ; !in_eof(); getline_simplify_cache() )
	{
	if(line[0] != '%')	/* If not a comment, then */
	    break;		/* comments section is done. */

	if(line[1] == '!')	/* we don't need extra magic lines */
	    continue;

	if(line[1] == '\0' || line[1] == ' ' || line[1] == '\t')
	    break;              /* %{space} terminates section */

	if(line_len > MAX_TOKENIZED)
	    break;              /* non-conforming line length terminates */

	if(line[1] == '%')
	    {
	    if(strcmp(line, "%%EndComments") == 0)
		{                   /* swallow "%%EndComments" */
		getline_simplify_cache();    /* but, we must leave a line in the buffer */
		break;
		}

	    /*
	    ** Look for signs of the begin of the Prolog.
	    ** MS-Windows 3.1 makes the mistake of starting the
	    ** Prolog without ending the header comments in any
	    ** of the permitted ways.
	    */
	    if(strncmp(line, "%%Begin", 7) == 0 || strncmp(line, "%%Include", 9) == 0)
		{
		warning(WARNING_PEEVE, _("Header comments section is unterminated"));
		break;
		}

	    if(header_trailer(FALSE))       /* see what this can make of it */
		continue;                   /* if it handled line, go for next */
	    }

	copy_comment(comments);		/* last resort, just pass it thru */
	} /* end of for loop */

    outermost_end(OUTERMOST_HEADER_COMMENTS);
    } /* end of read_header_comments() */

/*
** Read the document defaults into the -pages file.
** This is called by read_prolog() if a defaults section is detected.
*/
static void read_defaults(void)
    {
    outermost_start(OUTERMOST_DOCDEFAULTS);

    /* Write the "%%BeginDefaults" line to the -pages file. */
    fprintf(page_comments, "%s\n", line);

    /* Copy the rest of the document defaults section. */
    for(getline_simplify_cache(); ! in_eof(); getline_simplify_cache())
	{
	fprintf(page_comments, "%s\n", line);

	if(strncmp(line, "%%EndDefaults", 13) == 0)
	    {
	    getline_simplify_cache();      /* leave something for next guy */
	    break;
	    }

	/* Save this information for the benefit of ppr_split.c. */
	gu_sscanf(line, "%%%%PageMedia: %#s", sizeof(default_pagemedia), default_pagemedia);
	}

    outermost_end(OUTERMOST_DOCDEFAULTS);
    } /* end of read_defaults() */

/*
** Read the prolog and document setup section into the -text file.
** At the proper place, digress to read the document defaults
** into the "-pages" file.  When this funtion is called, there
** is already a line in the input buffer.
**
** We will stop when we hit the begining of the
** first page, the begining of the trailer, or end of the file.
**
** If there will be no need to read any pages, then return non-zero.
**
** If we find a document setup section or can create an empty one
** we set qentry.attr.docsetup to TRUE, otherwise we set it to FALSE.
**
** If the document has an %%EndSetup comment but no %%BeginSetup comment
** this code will not correct it.
*/
gu_boolean read_prolog(void)
    {
    int end_setup_seen = FALSE;		/* made true when "%%EndSetup" seen */
    qentry.attr.prolog = FALSE;		/* we may set true later */
    qentry.attr.docsetup = FALSE;	/* we may set true later */

    /*
    ** Because we can't be sure that we are in the prolog until and
    ** unless we see a "%%BeginProlog", we will not call
    ** outermost_start(OUTERMOST_PROLOG) here.  It might be that
    ** we will have to read the document defaults section before
    ** we get to the prolog.
    */

    while(! in_eof() )
	{
	if(line[0] == '%' && line[1] == '%')
	    {			/* if a DSC comment line, */
	    /*
	    ** If start of 1st page, start document setup
	    ** section if one does not exist.  Close document
	    ** setup section if it is not closed already.
	    */
	    if(nest_level() == 0 && strncmp(line, "%%Page:", 7) == 0)
		{
		if(!qentry.attr.prolog)		/* XV will be scolded here */
		    {
		    warning(WARNING_PEEVE, "No \"%%%%EndProlog\" before first \"%%%%Page:\", inserting one");
		    fputs("%%EndProlog\n", text);
		    qentry.attr.prolog = TRUE;
		    }

		/* If no document setup, make one */
		if(!qentry.attr.docsetup)
		    {
		    fputs("%%BeginSetup\n", text);
		    qentry.attr.docsetup = TRUE;
		    }

		/* If there was no %%EndSetup line, insert one and
		   leave a blank line after it. */
		if(!end_setup_seen)
		    fputs("%%EndSetup\n\n", text);

		return TRUE;	/* pages should be read */
		}

	    /*
	    ** If we have hit "%%Trailer" or "%%EOF", exit with a return
	    ** value which instructs main() not to call read_pages().
	    */
	    if(nest_level() == 0 && (strcmp(line, "%%Trailer") == 0 || strcmp(line, "%%EOF") == 0))
		return FALSE;

	    /*
	    ** If we see a document defaults section,
	    ** turn aside for a moment to copy it.
	    */
	    if(nest_level() == 0 && strncmp(line, "%%BeginDefaults", 15) == 0)
		{
		read_defaults();
		continue;
		}

	    /* Take note if we seen %%BeginSetup or %%EndSetup. */
	    if(nest_level() == 0 && strcmp(line, "%%BeginSetup") == 0)
		{
		qentry.attr.docsetup = TRUE;	/* just set flag */

		if(!qentry.attr.prolog)		/* if no "%%EndProlog" seen, */
		    {
		    warning(WARNING_SEVERE, "No \"%%%%EndProlog\" before \"%%%%BeginSetup\", inserting one");
		    outermost_end(OUTERMOST_PROLOG);
		    fputs("%%EndProlog\n", text);
		    qentry.attr.prolog = TRUE;
		    }

		outermost_start(OUTERMOST_DOCSETUP);
		}
	    else if( nest_level() == 0 && strcmp(line, "%%EndSetup") == 0 )
		{
		qentry.attr.docsetup = TRUE;
		end_setup_seen = TRUE;			/* just set flag */
		outermost_end(OUTERMOST_DOCSETUP);
		}

	    /* Look for "%%EndProlog", make note if we see it: */
	    if(nest_level() == 0 && strcmp(line, "%%EndProlog") == 0)
		{
		if(qentry.attr.prolog)	/* if already seen, */
		    {
		    warning(WARNING_SEVERE, "Extra \"%%%%EndProlog\"");
		    }
		else
		    {
		    qentry.attr.prolog = TRUE;
		    outermost_end(OUTERMOST_PROLOG);
		    }
		}

	    /*
	    ** If we see an explicit "%%BeginProlog", make a note of it:
	    ** Notice that we don't set qentry.attr.prolog = TRUE.
	    */
	    if(nest_level() == 0 && strcmp(line, "%%BeginProlog") == 0)
		outermost_start(OUTERMOST_PROLOG);

	    /* Unrecognized comment, just copy it. */
	    copy_comment(text);

	    /*
	    ** If we are in the document setup section, digress to read
	    ** media selection features and duplex selection features
	    ** and booklet and signature selection features.
	    **
	    ** By doing this we may be able to get a better idea of the
	    ** desired media or we may be able to emulate features which
	    ** are missing in the printer.
	    */
	    if(qentry.attr.docsetup && ((strncmp(line, "%%BeginFeature:", 15) == 0) || (strncmp(line, "%%IncludeFeature:", 17) == 0)))
		feature_spy();
	    }
	else                /* if not comment, just copy it */
	    {
	    fwrite(line, sizeof(unsigned char), line_len, text);
	    if(! line_overflow)
	    	fputc('\n', text);
	    }
	getline_simplify_cache();    /* get the next line */
	}

    /* no pages will follow, say so */
    return FALSE;
    } /* end of read_prolog() */

/*
** Start and end of page processing routines.
*/
void start_of_page_processing(void)
    {
    prepare_thing_bitmap();
    } /* end of start_of_page_processing() */

void end_of_page_processing(void)
    {
    dump_page_resources();
    dump_page_requirements();
    dump_page_media();
    } /* end of end_of_page_processing() */

/*
** Read in all the pages.  Return when we hit "%%Trailer"
** "%%EOF", or end of file, whichever comes first.
*/
void read_pages(void)
    {
    int pagetrailer = FALSE;
    int pageheader = TRUE;
    int script = FALSE;

    for( ; ! in_eof() ; getline_simplify_cache() )
	{
	if(line[0] == '%' && line[1] == '%' && nest_level() == 0 && line_len <= MAX_TOKENIZED)
	    {
	    if(strncmp(line, "%%Page:", 7) == 0)
		{
		if(!script)
		    {
		    outermost_start(OUTERMOST_SCRIPT);
		    script = TRUE;
		    }
		if(pagenumber++ > 0)		/* if not start of 1st page, */
		    end_of_page_processing();	/* close old page */
		start_of_page_processing();	/* now, start new page */
		pageheader = TRUE;		/* we in header now */
		pagetrailer = FALSE;		/* certainly not in trailer */
		fprintf(page_comments, "%s\n", line);
		fprintf(page_comments, "Offset: %ld\n", ftell(text));
		fprintf(text,"%s\n", line);
		continue;
		}
	    if(strcmp(line, "%%BeginPageSetup") == 0)
		{               /* this terminates page header */
		pageheader = FALSE;
		fprintf(text,"%s\n",line);
		continue;
		}
	    if(nest_level() == 0 && ( strcmp(line, "%%Trailer") == 0
		    || strcmp(line,"%%EOF") == 0 ) )
		{
		if(pagenumber > 0)
		    end_of_page_processing();
		return;
		}
	    if(strcmp(line, "%%PageTrailer") == 0)
		{
		pagetrailer = TRUE;
		fprintf(text, "%s\n", line);
		continue;
		}

	    if(tokens[1] && strcmp(tokens[1], "atend") == 0)
		continue;       /* no use for (atend) comments */

	    /* Call page comment processor. */
	    if(pagelevel() == 0)
		{
		if(pageheader || pagetrailer)
		    fprintf(page_comments, "%s\n", line);
		else
		    fprintf(text, "%s\n", line);
		}
	    }
	else                                    /* not DSC comment */
	    {
	    pageheader = FALSE;
	    fwrite(line, sizeof(unsigned char), line_len, text);
	    if(!line_overflow)
	    	fputc('\n', text);
	    }
	} /* end of for which loops `til broken or `til end of file */

	/* If we hit end of file, finish processing of last page. */
	if(pagenumber > 0)
	    end_of_page_processing();
    } /* end of read_pages() */

/*
** Read the document trailer.
** We will write the text to -text and process the comments,
** writing those we don't recognize to -comments.
** Those we do recognize are handled by header_trailer().
** Return when the %%EOF line is encountered.
*/
void read_trailer(void)
    {
    outermost_start(OUTERMOST_TRAILER);

    for(getline_simplify_cache(); ! in_eof(); getline_simplify_cache())
	{
	if(line[0]=='%' && line[1]=='%' && line_len <= MAX_TOKENIZED)
	    {				/* if DSC comment */
	    if(nest_level() == 0 && strcmp(line, "%%EOF") == 0)
		break;			/* stop on unnested %%EOF */

	    if(header_trailer(TRUE))	/* call trailer comment handler */
		continue;               /* if it returns true, it did it */

	    copy_comment(comments);     /* if unrecognized, save it */
	    }
	else                            /* not DSC comment */
	    {                           /* just print the line */
	    fwrite(line, sizeof(unsigned char), line_len, text);
	    if(!line_overflow)
	    	fputc('\n',text);
	    }
	}

    outermost_end(OUTERMOST_TRAILER);
    } /* end of read_trailer() */

/* end of file */

