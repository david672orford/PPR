/*
** mouse:~ppr/src/libuprint/claim_remote.c
** Copyright 1995--2003, Trinity College Computing Center.
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
** Last modified 5 November 2003.
*/

#include "before_system.h"
#include <errno.h>
#include <string.h>
#include <ctype.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "uprint.h"
#include "uprint_private.h"

/*
** Table for "remote system type ="
**
** The version number are minimums, any version higher or equal
** will match.
*/
struct	{
		const char *name;
		float version;
		gu_boolean osf_extensions;
		gu_boolean solaris_extensions;
		gu_boolean ppr_extensions;
		} systems[] =
		{
		{"BSD", 4.2, FALSE, FALSE, FALSE},
		{"SUNOS", 0.00, FALSE, FALSE, FALSE},
		{"SUNOS", 5.00, FALSE, TRUE, FALSE},
		{"Solaris", 1.0, FALSE, FALSE, FALSE},
		{"Solaris", 2.0, FALSE, TRUE, FALSE},
		{"PPR", 1.00, FALSE, FALSE, FALSE},
		{"PPR", 1.32, TRUE, FALSE, TRUE},
		{"PPR", 1.40, TRUE, TRUE, TRUE},
		{"WinNT", 3.10, FALSE, FALSE, FALSE},
		{"RedHat", 0.00, FALSE, FALSE, FALSE},
		{NULL, 0.0, FALSE, FALSE, FALSE}
		};

/*
** This function test whether the specified queue is among those
** defined in uprint-remote.conf.  If it is it returns TRUE.  For
** certain error conditions it calls uprint_error_callback() and
** sets uprint_errno.
**
** One problem is the question of how bad the matched entry has
** to be before we return false.
*/
gu_boolean printdest_claim_remote(const char dest[], struct REMOTEDEST *scratchpad)
	{
	FILE *conf;
	int linenum = 0;
	char line[256];
	size_t destlen;
	char *name, *value;

	DODEBUG(("uprint_claim_remote(dest = \"%s\", scratchpad = ?)", dest != (const char *)NULL ? dest : ""));

	if(dest == (const char *)NULL)
		{
		uprint_errno = UPE_BADARG;
		return FALSE;
		}

	/* If no uprint.conf file, the queue isn't ours. */
	if((conf = fopen(UPRINTREMOTECONF, "r")) == (FILE *)NULL)
		{
		if(errno != ENOENT)
			{
			uprint_errno = UPE_BADCONFIG;
			uprint_error_callback(_("Can't open \"%s\", errno=%d (%s)."), UPRINTREMOTECONF, errno, gu_strerror(errno));
			}
		else
			{
			uprint_errno = UPE_UNDEST;	/* unknown destination */
			}
		return FALSE;
		}

	destlen = strlen(dest);
	scratchpad->node = NULL;
	scratchpad->printer[0] = '\0';
	scratchpad->osf_extensions = FALSE;
	scratchpad->solaris_extensions = FALSE;
	scratchpad->ppr_extensions = FALSE;

	while(TRUE)
		{
		/* If ran off end of file, the queue isn't
		   in uprint-remote.conf. */
		if(fgets(line, sizeof(line), conf) == (char *)NULL)
			{
			uprint_errno = UPE_NONE;
			fclose(conf);
			return FALSE;
			}

		linenum++;

		/* Ignore lines that aren't section headings. */
		if(line[0] != '[')
			continue;

		/* Turn the "]" into a line termination. */
		line[strcspn(line, "]")] = '\0';

		/* Compare the destionation name to the shell
		   pattern in this line. */
		if(!gu_wildmat(dest, line+1))
			continue;

		/* All tests passed, it is a match! */

		/* If it is within the length limit, copy the
		   requested name to the remote queue name so
		   that it will be the default. */
		if(strlen(dest) <= LPR_MAX_QUEUE)
			strcpy(scratchpad->printer, dest);

		/* Read the lines in this section. */
		while(fgets(line, sizeof(line), conf))
			{
			linenum++;

			/* Ignore comment lines. */
			if(line[0] == '#' || line[0] == ';')
				continue;

			/* If we hit the next section, stop. */
			if(line[0] == '[')
				break;

			/* Lines will be in the form "name=value".
			   Extract name and value.  We will
			   eliminate spaces before the name, within
			   the name, before and after the "=", and
			   at the end of the line. */
			{
			char *si, *di;
			int c;

			for(si=di=line; (c=*(si++)) && c != '='; )
				{
				if(! isspace(c))
					*(di++) = tolower(c);
				}
			*di = '\0';
			name = line;

			for(; isspace(*si); si++);
			value = si;
			for(si=&value[strlen(value)]; --si >= value && isspace(*si); ) *si = '\0';
			}

			/* an action for each possible name */
			if(strcmp(name, "remotehost") == 0)
				{
				if(scratchpad->node) gu_free(scratchpad->node);
				scratchpad->node = gu_strdup(value);
				}

			else if(strcmp(name, "remoteprinter") == 0)
				{
				if(strlen(value) > LPR_MAX_QUEUE)
					{
					uprint_errno = UPE_BADCONFIG;
					uprint_error_callback(_("Value of \"%s\" too long in \"%s\" section \"[%s]\", line %d."), "remoteprinter", UPRINTREMOTECONF, dest, linenum);
					}
				else
					{
					strcpy(scratchpad->printer, value);
					}
				}

			else if(strcmp(name, "osfextensions") == 0)
				{
				int answer;
				if((answer = gu_torf(value)) == ANSWER_UNKNOWN)
					{
					uprint_errno = UPE_BADCONFIG;
					uprint_error_callback(_("Value for \"%s\" must be boolean in \"%s\" section \"[%s]\", line %d."), "osfextensions", UPRINTREMOTECONF, dest, linenum);
					}
				else
					{
					scratchpad->osf_extensions = answer ? TRUE : FALSE;
					}
				}

			else if(strcmp(name, "solarisextensions") == 0)
				{
				int answer;
				if((answer = gu_torf(value)) == ANSWER_UNKNOWN)
					{
					uprint_errno = UPE_BADCONFIG;
					uprint_error_callback(_("Value for \"%s\" must be boolean in \"%s\" section \"[%s]\", line %d."), "solarisextensions", UPRINTREMOTECONF, dest, linenum);
					}
				else
					{
					scratchpad->solaris_extensions = answer ? TRUE : FALSE;
					}
				}

			else if(strcmp(name, "pprextensions") == 0)
				{
				int answer;
				if((answer = gu_torf(value)) == ANSWER_UNKNOWN)
					{
					uprint_errno = UPE_BADCONFIG;
					uprint_error_callback(_("Value for \"%s\" must be boolean in \"%s\" section \"[%s]\", line %d."), "pprextensions", UPRINTREMOTECONF, dest, linenum);
					}
				else
					{
					scratchpad->ppr_extensions = answer ? TRUE : FALSE;
					}

				}

			else if(strcmp(name, "remotesystemtype") == 0)
				{
				char system[17];
				float version;
				int x;
				gu_boolean match = FALSE;

				if(sscanf(value, "%16s %f", system, &version) != 2)
					{
					uprint_errno = UPE_BADCONFIG;
					uprint_error_callback(_("Wrong format for \"systemtype\" value in \"%s\" section \"[%s]\", line %d."), UPRINTREMOTECONF, dest, linenum);
					}

				else
					{
					for(x=0; systems[x].name; x++)
						{
						if(gu_strcasecmp(system, systems[x].name) == 0
								&& version >= systems[x].version)
							{
							scratchpad->osf_extensions = systems[x].osf_extensions;
							scratchpad->solaris_extensions = systems[x].solaris_extensions;
							scratchpad->ppr_extensions = systems[x].ppr_extensions;
							match = TRUE;
							}
						}
					if(!match)
						{
						uprint_errno = UPE_BADCONFIG;
						uprint_error_callback(_("Unrecognized system \"%s\" %.2f in \"%s\" section \"[%s]\", line %d."), system, version, UPRINTREMOTECONF, dest, linenum);
						}
					}
				}

			else
				{
				/* don't discourage undefined lines */
				}

			} /* end of loop to read section */

		break;
		} /* end of loop to read whole file */

	fclose(conf);

	if(scratchpad->node == NULL || scratchpad->printer[0] == '\0')
		{
		uprint_errno = UPE_BADCONFIG;
		uprint_error_callback(_("Invalid section for queue \"%s\" in \"%s\"."), dest, UPRINTREMOTECONF);
		if(scratchpad->node) gu_free(scratchpad->node);
		return FALSE;
		}

	return TRUE;
	} /* end of printdest_claim_remote() */

/* end of file */

