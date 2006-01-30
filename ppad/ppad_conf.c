/*
** mouse:~ppr/src/ppad/ppad_conf.c
** Copyright 1995--2006, Trinity College Computing Center.
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
** Last modified 27 January 2006.
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

/* Convert a queue type to the appropriate configuration file directory path. */
static const char *conf_directory(enum QUEUE_TYPE queue_type)
	{
	switch(queue_type)
		{
		case QUEUE_TYPE_PRINTER:
			return PRCONF;
		case QUEUE_TYPE_GROUP:
			return GRCONF;
		case QUEUE_TYPE_ALIAS:
			return ALIASCONF;
		}
	}

/** Open a PPR destination configuration file.
 */
struct CONF_OBJ *conf_open(enum QUEUE_TYPE queue_type, const char destname[], int flags)
	{
	const char function[] = "conf_open";
	struct CONF_OBJ *obj = gu_alloc(1, sizeof(struct CONF_OBJ));

	obj->queue_type = queue_type;
	obj->name = destname;
	obj->flags = flags;
	obj->in = NULL;
	obj->out = NULL;
	obj->line = NULL;
	obj->line_space = 80;

	ppr_fnamef(obj->in_name, "%s/%s", conf_directory(queue_type), destname);

	if(flags & CONF_MODIFY)				/* modify existing, possibly create */
		{
		if(debug_level >= 1)
			{
			if(flags & CONF_CREATE)
				printf("Opening \"%s\" (create new or modify existing).\n", obj->in_name);
			else
				printf("Opening \"%s\" (modify existing).\n", obj->in_name);
			}

		do	{
			/* must open for update if we want an exclusive lock */
			if(!(obj->in = fopen(obj->in_name, "r+")))
				{
				if(errno == ENOENT)		/* if open failed because doesn't exist, */
					break;
				else
					fatal(EXIT_INTERNAL, _("%s(): %s(\"%s\", \"%s\") failed, errno=%d (%s)"), function, "fopen", obj->in_name, "r+", errno, gu_strerror(errno));
				}

			if(gu_lock_exclusive(fileno(obj->in), FALSE))	/* if lock failed, */
				{
				fclose(obj->in);
				obj->in = NULL;
				printf(_("Waiting for lock to clear.\n"));
				sleep(1);
				}
			} while(!obj->in);
		}
	else if(flags & CONF_CREATE)		/* create only, no modify existing */
		{
		struct stat statbuf;
		if(debug_level >= 1)
			printf("Opening \"%s\" (create new).\n", obj->in_name);
		if(stat(obj->in_name, &statbuf) == 0)
			{
			switch(obj->queue_type)
				{
				case QUEUE_TYPE_PRINTER:
					fprintf(errors, _("The printer \"%s\" already exists.\n"), destname);
					break;
				case QUEUE_TYPE_GROUP:
					fprintf(errors, _("The group \"%s\" already exists.\n"), destname);
					break;
				case QUEUE_TYPE_ALIAS:
					fprintf(errors, _("The alias \"%s\" already exists.\n"), destname);
					break;
				}
			gu_free(obj);
			obj = NULL;
			}
		}
	else								/* read-only */
		{
		if(flags & CONF_CREATE)
			printf("Opening \"%s\" (read only).\n", obj->in_name);
		if(!(obj->in = fopen(obj->in_name, "r")))
			{
			if(errno != ENOENT)
				fatal(EXIT_INTERNAL, _("%s(): %s(\"%s\", \"%s\") failed, errno=%d (%s)"), function, "fopen", obj->in_name, "r", errno, gu_strerror(errno));
			}
		}

	/* If we are modifying the file, we must create a temporary file for the new version. */
	if(flags & CONF_MODIFY || flags & CONF_CREATE)
		{
		/* Create temporary file in same dir, hidden (from pprd). */
		ppr_fnamef(obj->out_name,"%s/.ppad%ld", conf_directory(queue_type), (long)getpid());
		if(!(obj->out = fopen(obj->out_name, "w")))
			fatal(EXIT_INTERNAL, _("Can't open temporary file \"%s\" for write, errno=%d (%s)"), obj->out_name, errno, gu_strerror(errno));
		}

	/* If we didn't find it and it is not OK to create it, we have failed. */
	if(!obj->in && !(flags & CONF_CREATE))
		{
		if(flags & CONF_ENOENT_PRINT)
			{
			switch(queue_type)
				{
				case QUEUE_TYPE_PRINTER:
					fprintf(errors, _("The printer \"%s\" does not exist.\n"), destname);
					break;
				case QUEUE_TYPE_GROUP:
					fprintf(errors, _("The group \"%s\" does not exist.\n"), destname);
					break;
				case QUEUE_TYPE_ALIAS:
					fprintf(errors, _("The alias \"%s\" does not exist.\n"), destname);
					break;
				}
			}
		gu_free(obj);
		obj = NULL;
		}

	return obj;
	} /* end of conf_open() */

/*
** Read a line from the configuration file.
** If we suceed, return the line.
*/
char *conf_getline(struct CONF_OBJ *obj)
	{
	if(!obj->in)
		return NULL;
	
	if(!(obj->line = gu_getline(obj->line, &(obj->line_space), obj->in)))
		return NULL;

	if(debug_level >= 3)
		printf("<%s\n", obj->line);

	return obj->line;
	} /* end of confread() */

/*
** Write a line to the temporary file.
*/
int conf_printf(struct CONF_OBJ *obj, const char format_str[], ...)
	{
	va_list va;
	int retval;
	va_start(va, format_str);
	retval = conf_vprintf(obj, format_str, va);
	va_end(va);
	return retval;
	} /* end of conf_printf() */

int conf_vprintf(struct CONF_OBJ *obj, const char format_str[], va_list va)
	{
	if(!obj->out)
		fatal(EXIT_INTERNAL, "conf_write_vprintf(): internal error, file not open for modify");

	if(debug_level >= 3)
		{
		va_list our_copy;
		va_copy(our_copy, va);
		fputc('>', stdout);
		vfprintf(stdout, format_str, our_copy);
		va_end(our_copy);
		}

	vfprintf(obj->out, format_str, va);

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
int conf_close(struct CONF_OBJ *obj)
	{
	const char *function = "conf_close";
	if(obj->out)
		{
		struct stat statbuf;
		if(debug_level >= 1)
			printf("Saving new \"%s\".\n", obj->in_name);

		/* Reduce race condition time!	(See below.) */
		fflush(obj->out);

		/* Copy the protections from the old to the new configuration file because the user execute
		   bit tells if the printer is stopt and the other execute bit tells if it is protected.

		   !!! There is a possibility that pprd will modify the attributes in the split second
		   between when we read them and when we move the new file into place.

		   It will be good to get rid of this nonsense.
		   
		   */
		if(obj->in)		/* if not new, */
			{
			if(fstat(fileno(obj->in), &statbuf) < 0)
				fatal(EXIT_INTERNAL, _("%s(): %s() failed, errno=%d (%s)"), function, "fstat", errno, gu_strerror(errno));
			if(fclose(obj->in) == EOF)
				fatal(EXIT_INTERNAL, _("%s(): %s() failed, errno=%d (%s)"), function, "fclose", errno, gu_strerror(errno));
			if(chmod(obj->out_name, statbuf.st_mode) < 0)
				fatal(EXIT_INTERNAL, _("%s(): %s() failed, errno=%d (%s)"), function, "chmod", errno, gu_strerror(errno));
			}

		if(fclose(obj->out) == EOF)
			fatal(EXIT_INTERNAL, _("%s(): %s() failed, errno=%d (%s)"), function, "fclose", errno, gu_strerror(errno));

		/* Replace old with new. */
		if(rename(obj->out_name, obj->in_name) < 0)
			fatal(EXIT_INTERNAL, _("%s(): %s() failed, errno=%d (%s)"), function, "rename", errno, gu_strerror(errno));
		}

	else
		{
		if(debug_level >= 1)
			printf("Closing \"%s\".\n", obj->in_name);
		fclose(obj->in);
		}

	/* Should we tell pprd that the file has been changed?  Notice that
	 * this is allowed even in read-only mode in order to support "ppad touch".
	 */ 
	if(obj->flags & CONF_RELOAD)
		{
		switch(obj->queue_type)
			{
			case QUEUE_TYPE_PRINTER:
				write_fifo("NP %s\n", obj->name);
				break;
			case QUEUE_TYPE_GROUP:
				write_fifo("NG %s\n", obj->name);
				break;
			case QUEUE_TYPE_ALIAS:
				/* pprd doesn't know about these */
				break;
			}
		}

	gu_free_if(obj->line);
	gu_free(obj);

	return 0;
	} /* end of conf_close() */

/*
** Close the configuration file and delete the new copy.
**
** This is used to abort the modification of the
** configuration file.
*/
int conf_abort(struct CONF_OBJ *obj)
	{
	if(obj->out)
		{
		if(debug_level >= 1)
			printf("Discarding changes to \"%s\".\n", obj->in_name);
		fclose(obj->in);
		fclose(obj->out);
		unlink(obj->out_name);
		}

	gu_free_if(obj->line);
	gu_free(obj);

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
int conf_set_name(enum QUEUE_TYPE queue_type, const char queue_name[], int extra_flags, const char name[], const char value[], ...)
	{
	int name_len = strlen(name);
	struct CONF_OBJ *obj;
	char *line;

	if( ! am_administrator() )
		return EXIT_DENIED;

	/* Open the printer or group configuration file. */
	if(!(obj = conf_open(queue_type, queue_name, CONF_MODIFY | CONF_ENOENT_PRINT | extra_flags)))
		return EXIT_BADDEST;

	/* Copy up to but now including the first instance of
	   the line to be changed. */
	while((line = conf_getline(obj)))
		{
		if(strncmp(line, name, name_len) == 0 && line[name_len] == ':')
			break;
		conf_printf(obj, "%s\n", line);
		}

	/* If the new value is non-NULL, write it. */
	if(value)
		{
		va_list va;
		conf_printf(obj, "%s: ", name);
		va_start(va, value);
		conf_vprintf(obj, value, va);
		va_end(va);
		conf_printf(obj, "\n");
		}

	/* Copy any remaining lines, while deleting any furthur
	   instances of the line that was changed. */
	while((line = conf_getline(obj)))
		{
		if(strncmp(line, name, name_len) == 0 && line[name_len] == ':')
			continue;
		conf_printf(obj, "%s\n", line);
		}

	/* Commit changes */
	conf_close(obj);

	return EXIT_OK;
	} /* end of conf_set_name() */

/** Duplicate a PPR destination.
 */
int conf_copy(enum QUEUE_TYPE queue_type, const char from[], const char to[])
	{
	int retval = EXIT_OK;
	struct CONF_OBJ *from_obj = NULL, *to_obj = NULL;
	char *line;

	do	{
		if(!(from_obj = conf_open(queue_type, from, CONF_ENOENT_PRINT)))
			{
			retval = EXIT_BADDEST;
			break;
			}

		if(!(to_obj = conf_open(queue_type, to, CONF_CREATE)))
			{
			retval = EXIT_BADDEST;
			break;
			}

		while((line = conf_getline(from_obj)))
			conf_printf(to_obj, "%s\n", line);

		} while(FALSE);

	if(to_obj)
		conf_close(to_obj);
	if(from_obj)
		conf_close(from_obj);

	return retval;
	} /* conf_copy() */

/* end of file */

