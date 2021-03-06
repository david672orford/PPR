/*
** mouse:~ppr/src/ppad/ppad_conf.c
** Copyright 1995--2006, Trinity College Computing Center.
** Written by David Chappell.
**
** This file is part of PPR.  You can redistribute it and modify it under the
** terms of the revised BSD licence (without the advertising clause) as
** described in the accompanying file LICENSE.txt.
**
** Last modified 22 May 2006.
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
	#warning Expect spurious warning on next line
	}

/** Open a PPR destination (printer, group, or alias) configuration file.
 * queue_type is one of
 * 		QUEUE_TYPE_PRINTER
 * 		QUEUE_TYPE_GROUP
 * 		QUEUE_TYPE_ALIAS
 * flags is the bitwise or of
 * 		CONF_CREATE	-- create new, fail if exists and CONF_MODIFY is not defined
 * 		CONF_MODIFY -- modify existing, fail if does not exist and CONF_CREATE is not defined
 * 		CONF_RELOAD -- inform pprd of change (even if open mode did not allow modification)
 * 		CONF_ENOENT_PRINT -- print an error message if doesn't exist
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

	if(debug_level >= 1)
		{
		gu_utf8_printf(
			X_("Opening \"%s\" (create=%s, modify=%s, reload=%s).\n"),
			obj->in_name,
			flags & CONF_CREATE ? "YES" : "NO",
			flags & CONF_MODIFY ? "YES" : "NO",
			flags & CONF_RELOAD ? "YES" : "NO"
			);
		}

	/* If modification of existing files is allowed, we must try to open
	 * an existing file and lock it if we find one.
	 */
	if(flags & CONF_MODIFY)
		{
		do	{
			/* Though we do not modify the original file, we must open it
			 * for update if we want an exclusive lock. */
			if(!(obj->in = fopen(obj->in_name, "r+")))
				{
				if(errno == ENOENT)		/* if open failed because doesn't exist, */
					break;				/* drop out without a file object */
				else
					gu_Throw(_("%s(): %s(\"%s\", \"%s\") failed, errno=%d (%s)"), function, "fopen", obj->in_name, "r+", errno, gu_strerror(errno));
				}

			if(gu_lock_exclusive(fileno(obj->in), FALSE))	/* if lock failed, */
				{
				fclose(obj->in);
				obj->in = NULL;
				gu_utf8_printf(_("Waiting for lock to clear.\n"));
				sleep(1);
				}
			} while(!obj->in);
		}

	/* read-only or create only */
	else
		{
		if(!(obj->in = fopen(obj->in_name, "r")))
			{
			if(errno != ENOENT)
				gu_Throw(_("%s(): %s(\"%s\", \"%s\") failed, errno=%d (%s)"), function, "fopen", obj->in_name, "r", errno, gu_strerror(errno));
			}
		}

	/* If create only mode and file exists, we have a problem. */
	if(obj->in && (flags & CONF_CREATE) && !(flags & CONF_MODIFY))
		{
		switch(obj->queue_type)
			{
			case QUEUE_TYPE_PRINTER:
				gu_utf8_fprintf(stderr, _("The printer \"%s\" already exists.\n"), destname);
				break;
			case QUEUE_TYPE_GROUP:
				gu_utf8_fprintf(stderr, _("The group \"%s\" already exists.\n"), destname);
				break;
			case QUEUE_TYPE_ALIAS:
				gu_utf8_fprintf(stderr, _("The alias \"%s\" already exists.\n"), destname);
				break;
			}
		fclose(obj->in);
		obj->in = NULL;
		}

	/* If we tried to open the config file for read or for modify (in other
	 * words, if it is not OK to create it), but didn't find it, we have 
	 * failed. */
	else if(!obj->in && !(flags & CONF_CREATE))
		{
		if(flags & CONF_ENOENT_PRINT)
			{
			switch(queue_type)
				{
				case QUEUE_TYPE_PRINTER:
					gu_utf8_fprintf(stderr, _("The printer \"%s\" does not exist.\n"), destname);
					break;
				case QUEUE_TYPE_GROUP:
					gu_utf8_fprintf(stderr, _("The group \"%s\" does not exist.\n"), destname);
					break;
				case QUEUE_TYPE_ALIAS:
					gu_utf8_fprintf(stderr, _("The alias \"%s\" does not exist.\n"), destname);
					break;
				}
			}
		}

	/* If we are modifying the file or creating a new one, we must create a 
	 * temporary file for the new version. */
	else if((flags & CONF_MODIFY) || (flags & CONF_CREATE))
		{
		/* Create temporary file in same dir, hidden (from pprd). */
		ppr_fnamef(obj->out_name,"%s/.ppad%ld", conf_directory(queue_type), (long)getpid());
		if(!(obj->out = fopen(obj->out_name, "w")))
			gu_Throw(_("Can't open temporary file \"%s\" for write, errno=%d (%s)"), obj->out_name, errno, gu_strerror(errno));
		}

	/* If an error was detected above, */
	if(!obj->in && !obj->out)
		{
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
		gu_utf8_printf("<%s\n", obj->line);

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
		gu_putwc('>');
		gu_utf8_vfprintf(stdout, format_str, our_copy);
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
			gu_utf8_printf("Saving new \"%s\".\n", obj->in_name);

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
			gu_utf8_printf("Closing \"%s\".\n", obj->in_name);
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
			gu_utf8_printf("Discarding changes to \"%s\".\n", obj->in_name);
		fclose(obj->in);
		fclose(obj->out);
		unlink(obj->out_name);
		}

	gu_free_if(obj->line);
	gu_free(obj);

	return 0;
	} /* conf_abort() */

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

		/* Create a new configuration file for the copy.  CONF_RELOAD
		 * does no harm if the thing copied is an alias since conf_open()
		 * silently ignores it.
		 */
		if(!(to_obj = conf_open(queue_type, to, CONF_CREATE | CONF_RELOAD)))
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

