/*
** mouse:~ppr/src/libuprint/claim_bsd.c
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
** Last modified 18 February 2003.
*/

#include "before_system.h"
#include <string.h>
#include <ctype.h>
#include "gu.h"
#include "global_defines.h"
#include "uprint.h"
#include "uprint_private.h"

/*
** Return TRUE if the destname is the name of a valid LPR destination.
**
** In order to accomplish this, we search /etc/printcap.  This is not a
** complete or correct printcap(5) parser, it is just good enough for
** the cases we have met so far.  Here is a sample /etc/printcap:

lp|labprn:\
		:lp=:\
		:rm=labserver.trincoll.edu:\
		:rp=labprn:\
		:sd=/var/spool/lpd/labprn:\
		:lf=/var/spool/lpd/labprn/log:
		
lab-color:\
		:lp=:\
		:rm=labserver.trincoll.edu:\
		:rp=lab-color:\
		:sd=/var/spool/lpd/lab-color:\
		:lf=/var/spool/lpd/lab-color/log:
																				
*/
int printdest_claim_bsd(const char destname[])
	{
	if(uprint_lpr_installed())
		{
		FILE *f;

		if((f = fopen(uprint_lpr_printcap(), "r")) != (FILE*)NULL)
			{
			char *line = NULL;
			int line_len = 80;
			char *p, *fnbp;

			/* Loop through the lines.  The gu_getline() function can 
			   handle lines of any length and will strip trailing whitespace.
			   */
			while((line = gu_getline(line, &line_len, f)))
				{
				if(!line[0])			/* empty or whitespace-only lines */
					continue;

				/* Make a pointer to the first non-blank character.  This is to 
				   deal with a RedHat printconf-0.2 bug.  (It puts a single 
				   space in front of the first line of each printer configuration
				   that it generates.)
				   */
				fnbp = line + strspn(line, " \t");

				/* If the line begins with a pound sign, it is a comment
				   */
				if(fnbp[0] == '#')
					continue;
													   
				/* If the line _begins_ with a colon, it is a continuation 
				   line while probably has a lot for boring information for 
				   lpr about this queue.  We will skip lines like that.
				   */
				if(fnbp[0] == ':')
					continue;			 

				/* Find the first colon.  If there is none, there isn't a name
				   list here, so skip this line.  (The colon marks the end of
				   the list of names for this queue.)
				   */
				if((p = strchr(fnbp, ':')) == (char*)NULL)
					continue;

				*p = '\0';		/* truncate line at first colon */

				/* Iterate through a |-separated listed of queue names and aliases. */
				for(p = fnbp; (p = strtok(p, "|")); p = (char*)NULL)
					{
					if(strcmp(p, destname) == 0)
						{
						gu_free(line);
						fclose(f);
						return TRUE;
						}
					}

				} /* end of line loop */

			fclose(f);
			} /* If fopen() suceded */
		} /* if lpr installed */

	return FALSE;
	} /* end of printdest_claim_bsd() */

/* end of file */
