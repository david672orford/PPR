/*
** mouse: ~ppr/src/ppr/ppr_nest.c
** Copyright 1995--2001, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is" without
** express or implied warranty.
**
** Last modified 18 November 2002.
*/

#include "before_system.h"
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "ppr.h"
#include "ppr_exits.h"
#include "ppr_gab.h"

/*
** The functions in this module have a role in interpreting nested structures
** in DSC complient PostScript documents.  Nested structures include
** resources and documents.
**
** This also contains functions which track which part of the outermost
** document is being parsed.
*/

/*=========================================================================
** Code to keep track of what sort of nested structure we are inside
** of now.	A nested structure is one which can contain a complete
** valid DSC documente.	 Within a nested structure we ignore things
** such as "%%EOF".
=========================================================================*/

static int level = 0;
static int available_levels = 0;
static int *nest_stack = (int*)NULL;

/*
** This routine returns a string representing the type of
** a nesting level.
*/
static const char *str_nest_types(int nesttype)
	{
	switch(nesttype)
		{
		case NEST_RES: return "resource";
		case NEST_DOC: return "document";
		case NEST_BADEPS: return "bad EPS";
		case NEST_BADRES: return "bad resource";
		default: return "<INVALID>";
		}
	} /* end of str_nest_types() */

/*
** This function is called whenever a comment which indicates a new level
** of nesting is encountered.
*/
void nest_push(int leveltype, const char *name)
	{
	level++;

	if(option_gab_mask & GAB_STRUCTURE_NESTING)
		printf("nesting: new level: type=\"%s\", new level=%d (%s)\n", str_nest_types(leveltype), level, name != (char*)NULL ? name : "<NULL>");

	/* If we need more space, then allocate it. */
	if( level > available_levels )
		{
		available_levels += 10;
		nest_stack = (int*)gu_realloc(nest_stack, available_levels, sizeof(int));
		}
	nest_stack[level-1] = leveltype;
	} /* end of nest_push() */

/*
** This is called whenever a comment which indicates the end of a nested
** structure is encountered.
*/
void nest_pop(int leveltype)
	{
	if(--level < 0)
		fatal(PPREXIT_OTHERERR, "ppr_nest.c: nest_pop(): stack underflow");

	if(option_gab_mask & GAB_STRUCTURE_NESTING)
		printf("nesting: end level: type=\"%s\", popped by \"%s\", new level=%d\n", str_nest_types(nest_stack[level]), str_nest_types(leveltype), level);

	if(leveltype != nest_stack[level])
		{
		warning(WARNING_SEVERE, "Bad nesting, %s terminated as though it were a %s", str_nest_types(nest_stack[level]), str_nest_types(leveltype));
		level++;				/* try to ignore the problem by ignoring the offending line */
		}
	} /* end of nest_pop() */

/*
** This function returns an integer which indicates how many nested
** structures the current line is enclosed within.
*/
int nest_level(void)
	{
	return level;
	} /* end of nest_level() */

/*
** This returns a code which indicates the type of the innermost
** enclosing nested structure.
*/
int nest_inermost_type(void)
	{
	if(level == 0)
		return NEST_NONE;
	else
		return nest_stack[level-1];
	} /* end of nest_inermost_type() */

/*=======================================================================
** Functions to keep track of which section of the outermost document
** we are currently in.
=======================================================================*/

static int outermost = OUTERMOST_UNDEFINED;

/*
** This routine returns a string representing the type of
** a nesting level.
*/
const char *str_outermost_types(int nesttype)
	{
	switch(nesttype)
		{
		case OUTERMOST_UNDEFINED: return "UNDEFINED";
		case OUTERMOST_HEADER_COMMENTS: return "header comments";
		case OUTERMOST_PROLOG: return "prolog";
		case OUTERMOST_DOCDEFAULTS: return "defaults";
		case OUTERMOST_DOCSETUP: return "setup";
		case OUTERMOST_SCRIPT: return "script";
		case OUTERMOST_TRAILER: return "trailer";
		default: return "<INVALID>";
		}
	} /* end of str_outermost_types() */

/*
** This function is called to mark the start of a section of
** the outermost document.
*/
void outermost_start(int sectioncode)
	{
	if(option_gab_mask & GAB_STRUCTURE_SECTIONS)
		printf("sections: begining of %s\n", str_outermost_types(sectioncode));

	if(outermost == sectioncode)
		warning(WARNING_SEVERE, _("redundant Begin %s comment"), str_outermost_types(sectioncode));

	outermost = sectioncode;
	} /* end of outermost_start() */

/*
** This function is called to mark the end of a section of the outermost
** document.  It will get upset if we end a section that we did not begin.
** If this happens it is a bug in PPR.	We make one exception:	the
** prolog may ommit "%%BeginProlog".
*/
void outermost_end(int sectioncode)
	{
	if(option_gab_mask & GAB_STRUCTURE_SECTIONS)
		printf("sections: End %s\n", str_outermost_types(sectioncode));

	/*
	** If the section end comment doesn't coorespond to the section we are in,
	** complain, unless it is "%%EndProlog" withoug "%%StartProlog" (which
	** isn't required).
	*/
	if(sectioncode != outermost)
		{
		if(outermost == OUTERMOST_UNDEFINED)
			{
			if(sectioncode != OUTERMOST_PROLOG)
				warning(WARNING_SEVERE, _("maked end of %s section without marked begin"), str_outermost_types(sectioncode));
			}
		else
			{
			warning(WARNING_SEVERE, _("marked end of %s section in %s section"), str_outermost_types(sectioncode), str_outermost_types(outermost) );
			}
		}

	outermost = OUTERMOST_UNDEFINED;
	} /* end of outermost_end() */

/*
** This function returns a code which indicates which part of the
** outermost document is being parsed.
*/
int outermost_current(void)
	{
	return outermost;
	} /* end outermost_current() */

/* end file file */

