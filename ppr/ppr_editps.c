/*
** mouse:~ppr/src/ppr/ppr_editps.c
** Copyright 1995--2004, Trinity College Computing Center.
** Written by David Chappell.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**
** * Redistributions of source code must retain the above copyright notice,
** this list of conditions and the following disclaimer.
**
** * Redistributions in binary form must reproduce the above copyright
** notice, this list of conditions and the following disclaimer in the
** documentation and/or other materials provided with the distribution.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE
** LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
** CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
** SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
** INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
** CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
** ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
** POSSIBILITY OF SUCH DAMAGE.
**
** Last modified 20 February 2004.
*/

#include "config.h"
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"

#include "ppr.h"
#include "ppr_gab.h"
#include "ppr_exits.h"

/*
** This function is called to locate a header line within the first buffer of
** the PostScript job.  It returns the value on that line in a heap block.
*/
static char *find_header(const unsigned char *in_ptr, const char name[])
	{
	char *ptr;
	if((ptr = strstr((char*)in_ptr, name)))
		{
		int ptr_len;
		ptr += strlen(name);
		ptr += strspn(ptr, " \t");				/* eat leading space */
		ptr_len = strcspn(ptr, "\r\n");			/* find end of line */

		/* Eat trailing spaces. */
		while(ptr_len > 0 && isspace(ptr[ptr_len - 1]))
			ptr_len--;

		/* If PostScript quoted, remove quotes. */
		if(ptr_len > 1 && ptr[0] == '(' && ptr[ptr_len - 1] == ')')
			{
			ptr++;
			ptr_len -= 2;
			}

		ptr = gu_strndup(ptr, ptr_len);

		if(option_gab_mask & GAB_INFILE_EDITPS)
			printf(_("%sHeader \"%s\" value is \"%s\".\n"), "  ", name, ptr);
		}
	else
		{
		if(option_gab_mask & GAB_INFILE_EDITPS)
			printf(_("%sHeader \"%s\" not found.\n"), "	 ", name);
		}
	return ptr;
	}

/*
** This function is called if -H editps has been used.
**
** This function is invoked with an opening segment of the input
** file in a buffer.  The buffer is pointed to by in_ptr, the length
** of the file segment is indicated by in_left.
**
** This function will determine if the input file should be passed
** thru an automatic editor.  If it should, this function will
** return an argv[] style array.  It will likely make this
** determination by examining the "%%Creator:" line.
**
** When this function is called, it is guaranteed that in_ptr points to a
** NULL terminated string.
*/
const char **editps_identify(const unsigned char *in_ptr, int in_left)
	{
	/* These are the header fields we will use to make our descision. */
	char *creator;
	char *copyright;
	char *beginresource;

	/* These will be used for reading editps.conf. */
	const char filename[] = EDITPSCONF;
	FILE *f;
	char *line = NULL;
	int line_space = 80;
	int linenum = 0;

	/* These are the items on each configuration file line. */
	char *filter = NULL, *interpreter, *min_editps_level_str, *max_editps_level_str;
	char *ptr, *item, *value;
	int min_editps_level, max_editps_level;

	/* These are used to return the name of the selected filter. */
	gu_boolean matched = FALSE;
	const char **result = (const char **)NULL;
	static char *args[10];
	static char level_str[3];

	if(option_gab_mask & GAB_INFILE_EDITPS)
		printf(_("Option \"-H editps\" used, scanning \"%s\".\n"), filename);

	/* Get the values of various header fields. */
	creator = find_header(in_ptr, "%%Creator:");
	copyright = find_header(in_ptr, "%%Copyright:");
	beginresource = find_header(in_ptr, "%%BeginResource:");

	/*
	** Search the editps.conf file for an editor which applies to this
	** PostScript file.
	*/

	/* If the user uses the -H editps switch without first
	   creating an editps.conf file, it is fair to consider
	   if a fatal error. */
	if((f = fopen(filename, "r")) == (FILE*)NULL)
		fatal(PPREXIT_OTHERERR, _("Can't open \"%s\", errno=%d (%s)"), filename, errno, gu_strerror(errno));

	/*
	** Read lines until we get to the end of the file or until an identification
	** is made.
	*/
	while(!matched && (line = gu_getline(line, &line_space, f)))
		{
		linenum++;

		/* Skip blank lines and comment lines: */
		if(line[0] == '\0' || line[0] == '#' || line[0] == ';') continue;

		if(option_gab_mask & GAB_INFILE_EDITPS)
			printf(_("%sConsidering line: %s\n"), "  ", line);

		/*
		** Find the first 4 fields on the line.
		** The format of the line is like this:
		**
		** editps/Windows_NT_4.0:/usr/bin/perl:1:10:creator=Windows NT 4.0:
		*/
		ptr = line;
		if( !(filter = gu_strsep(&ptr, ":"))
				|| !(interpreter = gu_strsep(&ptr, ":"))
				|| !(min_editps_level_str = gu_strsep(&ptr, ":"))
				|| !(max_editps_level_str = gu_strsep(&ptr, ":")) )
			fatal(PPREXIT_OTHERERR, _("Insufficient parameters on \"%s\" line %d"), filename, linenum);

		/* Make sure all of the fields meet syntax and allowed range requirements. */
		if(!*filter)
			fatal(PPREXIT_OTHERERR, _("1st field is empty on \"%s\" line %d"), filename, linenum);
		if((min_editps_level = atoi(min_editps_level_str)) < 1 || min_editps_level > 10)
			fatal(PPREXIT_OTHERERR, _("3rd field is not in the range 1--10 on \"%s\" line %d"), filename, linenum);
		if((max_editps_level = atoi(max_editps_level_str)) < 1 || max_editps_level > 10)
			fatal(PPREXIT_OTHERERR, _("4th field is not in the range 1--10 on \"%s\" line %d"), filename, linenum);

		/* Make sure the minimum and maximum editps levels don't make a match always impossible. */
		if(!(min_editps_level <= max_editps_level))
			fatal(PPREXIT_OTHERERR, _("3rd field must be <= 4th field on \"%s\" line %d"), filename, linenum);

		/* If this editps filter can't do the requested level of editing, reject it. */
		if(option_editps_level < min_editps_level || option_editps_level > max_editps_level)
			{
			if(option_gab_mask & GAB_INFILE_EDITPS)
				printf(_("%sDesired editps level (%d) is not this range.\n"), "    ", option_editps_level);
			continue;
			}

		/*
		** The 5th and subsequent fields are points of identification which
		** must match the characteristics of the PostScript file before
		** an identification is considered to have been made.
		**
		** We treat these as quoted fields because they may need to match a colon.
		*/
		while((item = gu_strsep_quoted(&ptr, ":", NULL)))
			{
			if(!*item)		/* skip empty items (presumably at end) */
				continue;

			if(!(value = strchr(item, '=')))
				fatal(PPREXIT_OTHERERR, _("Condition \"%s\" contains no \"=\" on \"%s\" line %d"), item, filename, linenum);
			*value = '\0';
			value++;

			if(strcmp(item, "creator") == 0)
				{
				if(!creator || !gu_wildmat(creator, value))
					break;
				}
			else if(strcmp(item, "copyright") == 0)
				{
				if(!creator || !gu_wildmat(copyright, value))
					break;
				}
			else if(strcmp(item, "beginresource") == 0)
				{
				if(!creator || !gu_wildmat(beginresource, value))
					break;
				}
			else
				{
				fatal(PPREXIT_OTHERERR, _("Unrecognized type of identity proof \"%s\" at \"%s\" line %d"), item, filename, linenum);
				}
			}

		/*
		** If we ran out of points of identification, then an identification
		** has been made.  Now all we need to do is make sure the interpreter
		** (if specified) is available and it is a match!
		*/
		if(item)										/* if unmet conditions, */
			{
			if(option_gab_mask & GAB_INFILE_EDITPS)
				{
				item[strlen(item)] = '=';						/* put equals sign back */
				printf(_("%sCondition \"%s\" is not met.\n"), "    ", item);
				}
			}
		else
			{
			if(!*interpreter || access(interpreter, X_OK) != -1)
				{
				if(option_gab_mask & GAB_INFILE_EDITPS)
					printf(_("Match found in filter \"%s\".\n"), filter);
				matched = TRUE;
				}
			else
				{
				if(option_gab_mask & GAB_INFILE_EDITPS)
					printf(_("%sRequired interpreter \"%s\" is not available.\n"), "    ", interpreter);
				}
			}
		} /* end of line reading loop */

	/* close editps.conf */
	fclose(f);

	/* If we have a match on a section which
	   applies to this editps level, */
	if(matched)
		{
		args[0] = gu_strdup(filter);			/* <--- memory leak !!! */
		snprintf(level_str, sizeof(level_str), "%d", option_editps_level);
		#if 0
		args[1] = level_str;
		args[2] = (char*)NULL;
		#else
		args[1] = (char*)NULL;
		#endif
		result = (const char **)args;
		}
	else
		{
		if(option_gab_mask & GAB_INFILE_EDITPS)
			printf(_("No matching filter found.\n"));
		}

	if(line) gu_free(line);
	if(creator) gu_free(creator);
	if(copyright) gu_free(copyright);
	if(beginresource) gu_free(beginresource);

	return result;
	} /* end of editps_identify() */

/* end of file */

