/*
** mouse:~ppr/src/ppad/ppad_conf.c
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
** Last modified 11 February 2004.
*/

/*
** Part of the administrator's utility, this module contains code to
** edit printer and group configuration files.
*/

#include "config.h"
#include <stdio.h>
#include <sys/types.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "ppad.h"
#include "util_exits.h"

#define STATE_CLOSED 0			/* no file open */
#define STATE_NOMODIFY 1		/* file open for read */
#define STATE_MODIFY 2			/* file open for write */

static int state = STATE_CLOSED;
static FILE *confin;
static char confin_name[MAX_PPR_PATH];
static FILE *confout;
static char confout_name[MAX_PPR_PATH];
char *confline = NULL;
static int confline_len;

/*
** Open a configuration file.  This is called by prnopen()
** and grpopen() below.
*/
int confopen(enum QUEUE_TYPE queue_type, const char destname[], gu_boolean modify, gu_boolean create)
	{
	const char *confdir;

	switch(queue_type)
		{
		case QUEUE_TYPE_PRINTER:
			confdir = PRCONF;
			break;
		case QUEUE_TYPE_GROUP:
			confdir = GRCONF;
			break;
		case QUEUE_TYPE_ALIAS:
			confdir = ALIASCONF;
			break;
		}

	confin = NULL;

	ppr_fnamef(confin_name, "%s/%s", confdir, destname);

	if(debug_level >= 1)
		printf("Opening \"%s\" %s.\n", confin_name, modify ? "for edit" : "read-only");

	again:				/* <-- for locking retries */

	/*
	** If modifying the file, we must open it for read
	** and open a temporary file for write.
	*/
	if(modify)
		{
		/* must open for update if we want an exclusive lock */
		if((confin = fopen(confin_name, "r+")) == (FILE*)NULL)
			{
			if(!create)
				return -1;
			}
		else
			{
			if(gu_lock_exclusive(fileno(confin), FALSE))
				{
				fclose(confin);
				printf(_("Waiting for lock to clear.\n"));
				sleep(1);
				goto again;
				}
			}

		/* Create temporary file in same dir, hidden (from pprd). */
		ppr_fnamef(confout_name,"%s/.ppad%ld", confdir, (long)getpid());
		if((confout = fopen(confout_name, "w")) == (FILE*)NULL)
			fatal(EXIT_INTERNAL, _("Can't open temporary file \"%s\" for write, errno=%d (%s)"), confout_name, errno, gu_strerror(errno));

		state = STATE_MODIFY;
		}

	/*
	** If we will not be modifying the file, we need
	** only open it for read.
	*/
	else
		{
		if((confin = fopen(confin_name, "r")) == (FILE*)NULL)
			return -1;

		state = STATE_NOMODIFY;
		}

	confline_len = 128;
	confline = (char*)gu_alloc(confline_len, sizeof(char));

	return 0;
	} /* end of confopen() */

/*
** Open a printer configuration file.
*/
int prnopen(const char prnname[], gu_boolean modify)
	{
	return confopen(QUEUE_TYPE_PRINTER, prnname, modify, FALSE);
	} /* end of prnopen() */

/*
** Open a group configuration file.
*/
int grpopen(const char *grpname, gu_boolean modify, gu_boolean create)
	{
	return confopen(QUEUE_TYPE_GROUP, grpname, modify, create);
	} /* end of grpopen() */

/*
** Read a line from the configuration file.
** If we suceed, return TRUE.
** The line is returned in the global variable "confline[]".
*/
int confread(void)
	{
	const char function[] = "confread";
	int len;

	if(state == STATE_CLOSED)
		fatal(EXIT_INTERNAL, X_("%s(): attempt to read without open file"), function);

	if(!confin)
		return FALSE;
	
	if(fgets(confline, confline_len, confin) == (char*)NULL)
		return FALSE;

	len = strlen(confline);

	while(len == (confline_len - 1) && confline[len - 1] != '\n')
		{
		confline_len *= 2;
		confline = (char*)gu_realloc(confline, confline_len, sizeof(char));
		if(!fgets((confline + len), (confline_len - len), confin)) break;
		len = strlen(confline);
		}

	/* Remove trailing spaces. */
	while((--len >= 0) && isspace(confline[len]) )
		confline[len] = '\0';

	if(debug_level >= 3)
		printf("<%s\n", confline);

	return TRUE;
	} /* end of confread() */

/*
** Write a line to the temporary file.
*/
int conf_printf(const char format_str[], ...)
	{
	va_list va;
	int retval;
	va_start(va, format_str);
	retval = conf_vprintf(format_str, va);
	va_end(va);
	return retval;
	} /* end of conf_printf() */

int conf_vprintf(const char format_str[], va_list va)
	{
	if(state != STATE_MODIFY)
		fatal(EXIT_INTERNAL, "conf_write_vprintf(): internal error, file not open for modify");

	vfprintf(confout, format_str, va);

	if(debug_level >= 3)
		{
		fputc('>', stdout);
		vfprintf(stdout, format_str, va);
		}

	return 0;
	} /* end of conf_vprintf() */

/*
** Close the configuration file, replacing it with the temporary
** file if necessary.  (It is necessary if the configuration file
** was opened for modification.)
**
** Notice that if the file was open for modification, we use
** rename() to put the new version in place.  POSIX guarantees
** that rename() is atomic.  We used to unlink() to old file
** first but this is a bad idea because there is a brief
** window when no configuration file exists.  The result was
** that a series of ppad commands executed from a script
** would caused pprd to exit because the first command would
** modify the file and cause ppad to reload, but the open()
** for the reload would sometimes come between the unlink()
** and the rename().
*/
int confclose(void)
	{
	const char *function = "confclose";
	switch(state)
		{
		case STATE_CLOSED:
			fatal(EXIT_INTERNAL, "%s(): no file open", function);
		case STATE_NOMODIFY:
			if(debug_level >= 1)
				printf("Closing \"%s\".\n", confin_name);
			fclose(confin);
			state = STATE_CLOSED;
			break;
		case STATE_MODIFY:
			{
			struct stat statbuf;
			if(debug_level >= 1)
				printf("Saving new \"%s\".\n", confin_name);

			/* Reduce race condition time!	(See below.) */
			fflush(confout);

			/* Copy the protections from the old to the new
			   configuration file because the user execute
			   bit tells if the printer is stopt and the
			   other execute bit tells if it is protected.
			   !!! There is a possibility that pprd will
			   modify the attributes in the split second
			   between when we read them and when we move
			   the new file into place. */
			if(confin)
				{
				if(fstat(fileno(confin), &statbuf) < 0)
					fatal(EXIT_INTERNAL, _("%s(): %s() failed, errno=%d (%s)"), function, "fstat", errno, gu_strerror(errno));
				if(fclose(confin) == EOF)
					fatal(EXIT_INTERNAL, _("%s(): %s() failed, errno=%d (%s)"), function, "fclose", errno, gu_strerror(errno));
				if(chmod(confout_name, statbuf.st_mode) < 0)
					fatal(EXIT_INTERNAL, _("%s(): %s() failed, errno=%d (%s)"), function, "chmod", errno, gu_strerror(errno));
				}

			if(fclose(confout) == EOF)
				fatal(EXIT_INTERNAL, _("%s(): %s() failed, errno=%d (%s)"), function, "fclose", errno, gu_strerror(errno));

			/* Replace old with new. */
			if(rename(confout_name, confin_name) < 0)
				fatal(EXIT_INTERNAL, _("%s(): %s() failed, errno=%d (%s)"), function, "rename", errno, gu_strerror(errno));

			state = STATE_CLOSED;
			}
			break;
		}

	if(confline)
		{
		gu_free(confline);
		confline = NULL;
		}

	return 0;
	} /* end of confclose() */

/*
** Close the configuration file and delete the new copy.
**
** This is used to abort the modification of the
** configuration file.
*/
int confabort(void)
	{
	const char function[] = "confabort";
	switch(state)
		{
		case STATE_CLOSED:
			fatal(EXIT_INTERNAL, "%s(): internal error, no file open", function);
		case STATE_NOMODIFY:
			fatal(EXIT_INTERNAL, "%s(): internal error, not open in write mode", function);
		case STATE_MODIFY:
			if(debug_level >= 1)
				printf("Discarding changes to \"%s\".\n", confin_name);
			fclose(confin);
			fclose(confout);
			unlink(confout_name);
			state = STATE_CLOSED;
			break;
		}

	if(confline)
		{
		gu_free(confline);
		confline = NULL;
		}

	return 0;
	} /* end of confabort() */

/*
** This function takes care of a common case.  In this case
** we look for the first line which begins with a certain
** keyword.  We replace that line with a line with the
** specified value.  We then delete any remaining lines with
** the same keyword.  If there weren't any lines at all with
** the specified keyword, then we put it at the end of the file.
** If value is NULL, then this function serves to delete the
** keyword from the file.
*/
int conf_set_name(enum QUEUE_TYPE queue_type, const char queue_name[], const char name[], const char value[], ...)
	{
	int name_len = strlen(name);

	if( ! am_administrator() )
		return EXIT_DENIED;

	/* Open the printer or group configuration file. */
	if(confopen(queue_type, queue_name, TRUE, FALSE))
		{
		switch(queue_type)
			{
			case QUEUE_TYPE_PRINTER:
				fprintf(errors, _("The printer \"%s\" does not exist.\n"), queue_name);
				break;
			case QUEUE_TYPE_GROUP:
				fprintf(errors, _("The group \"%s\" does not exist.\n"), queue_name);
				break;
			case QUEUE_TYPE_ALIAS:
				fprintf(errors, _("The alias \"%s\" does not exist.\n"), queue_name);
				break;
			}
		return EXIT_BADDEST;
		}

	/* Copy up to but now including the first instance of
	   the line to be changed. */
	while(confread())
		{
		if(strncmp(confline, name, name_len) == 0 && confline[name_len] == ':')
			break;
		conf_printf("%s\n", confline);
		}

	/* If the new value is non-NULL, write it. */
	if(value)
		{
		va_list va;
		conf_printf("%s: ", name);
		va_start(va, value);
		conf_vprintf(value, va);
		va_end(va);
		conf_printf("\n");
		}

	/* Copy any remaining lines, while deleting any furthur
	   instances of the line that was changed. */
	while(confread())
		{
		if(strncmp(confline, name, name_len) == 0 && confline[name_len] == ':')
			continue;
		conf_printf("%s\n", confline);
		}

	/* Commit changes */
	confclose();

	return EXIT_OK;
	} /* end of conf_set_name() */

/* end of file */

