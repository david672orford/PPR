/*
** mouse:~ppr/src/libuprint/claim_sysv.c
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
** Last modified 18 November 2003.
*/

#include "before_system.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <memory.h>
#include <string.h>
#include <ctype.h>
#include "gu.h"
#include "global_defines.h"
#include "uprint.h"
#include "uprint_private.h"

/*
** Return TRUE if the destname is the name
** of a valid LP destination.
*/
int printdest_claim_sysv(const char destname[])
	{
	if(uprint_lp_installed())
		{
		char fname[MAX_PPR_PATH];
		struct stat statbuf;

		/*
		** Test for a group of printers which LP calls a "class".
		** Presumably we are looking for a file which contains
		** a list of the members.
		*/
		snprintf(fname, sizeof(fname), "%s/%s", uprint_lp_classes(), destname);
		if(stat(fname, &statbuf) == 0)
			{
			return TRUE;
			} /* classes */

		/*
		** Try for a printer.
		**
		** On most systems we are looking for the interface
		** program, but on Solaris 5.x we are looking for a
		** directory which contains configuration files.
		**
		** On SGI systems we have to contend with the program
		** "glp" and friends which go snooping in the LP
		** configuration directories to build a list of
		** printers.  The script sgi_glp_hack for each PPR queue
		** creates a file in /var/spool/lp/members
		** which contains something like "/dev/null" and
		** non-executable file begining with the string "#UPRINT"
		** in /var/spool/lp/interface.
		*/
		snprintf(fname, sizeof(fname), "%s/%s", uprint_lp_printers(), destname);
		if(stat(fname, &statbuf) == 0)
			{
			if(S_ISDIR(statbuf.st_mode))		/* Solaris 5.x */
				return TRUE;
			if(statbuf.st_mode & S_IXUSR)		/* Executable */
				return TRUE;

			/*
			** Check for the string "#UPRINT" at the start of the
			** first line and return FALSE if we find it since
			** its presence would indicate that it is not a real
			** printer but rather a fake one we made for some reason
			** such as to satisfy SGI printing utilities.
			*/
			{
			int f;
			if((f = open(fname, O_RDONLY)) != -1)
				{
				char magic[7];
				int compare = -1;

				if(read(f, magic, 7) == 7)
					compare = memcmp(magic, "#UPRINT", 7);

				close(f);

				if(compare == 0)
					return FALSE;
				}
			}

			return TRUE;
			} /* printers */

		/*
		** If the macro specifying its name is defined, try the
		** Solaris 2.6 printing system configuration file.
		*/
		if(uprint_lp_printers_conf())
			{
			FILE *f;

			if((f = fopen("/etc/printers.conf", "r")))
				{
				char line[256];
				char *p;
		
				while(fgets(line, sizeof(line), f))
					{
					if(isspace(line[0]))
						continue;

					if((p = strchr(line, ':')) == (char*)NULL)
						continue;

					*p = '\0';

					if(strcmp(line, destname) == 0)
						{
						fclose(f);
						return TRUE;
						}
					}

				fclose(f);
				}
			} /* printers.conf */

		} /* lp installed */

	return FALSE;
	} /* end of printdest_claim_sysv() */

/* end of file */
