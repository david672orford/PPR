/*
** mouse:~ppr/src/libpprdb/gdbm_dbauth.c
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
** Last modified 19 April 2001.
*/

#include "before_system.h"
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <gdbm.h>
#include <errno.h>
#include "gu.h"
#include "global_defines.h"
#include "userdb.h"

extern gdbm_error gdbm_errno;	/* needed for gdbm 1.5 */

/*
** derr(), a dummy error handler for gdbm
*/
#ifdef __cplusplus
extern "C" void derr1(...);
void derr1(...) { }
#else
static void derr1(char *s) { }
#endif

/*
** Authorization function, return the user's record and indicate if printing
** can be carried out in consideration of the amount of money currently
** in the account.
*/
enum USERDB_RESULT db_auth(struct userdb *entry_copy, const char *username)
	{
	GDBM_FILE dbfile;
	datum key;
	datum data;
	enum USERDB_RESULT ret = USER_OK;

	if(!username)
		return USER_ERROR;

	if((dbfile = gdbm_open(DBNAME,0,GDBM_READER,S_IRUSR|S_IWUSR,derr1)) == (GDBM_FILE)NULL)
		{
		error("db_auth(): opening database \"%s\", errno=%d (%s), gdbm_errno=%d",DBNAME,errno,gu_strerror(errno),gdbm_errno);
		return USER_ERROR;
		}

	key.dptr = dbstrlower(username);	/* key is user name */
	key.dsize = strlen(username);		/* converted to lower case */

	data = gdbm_fetch(dbfile, key);		/* get the record */
	gdbm_close(dbfile);

	if(data.dptr == (char*)NULL)		/* if not found */
		{
		ret = USER_ISNT;				/* just tell the caller */
		}
	else
		{
		/* copy into caller's block */
		memcpy(entry_copy, data.dptr, sizeof(struct userdb));

		/* free block alloced by gdbm_fetch() */
		free(data.dptr);				/* !!! */

		/*
		** If the user is overdrawn and it is during business hours,
		** then revoke the user's credit.  Business hours are defined
		** as not Saturday or Sunday and between 9am (inclusive) and
		** 5pm (exclusive).
		*/
		if(entry_copy->balance <= entry_copy->cutoff)
			{
#ifdef BUSINESS_HOURS
			time_t rawnow;			  /* seconds since 1 Jan 1970 */
			struct tm *now;			  /* broken into day, hour, etc. */

			time(&rawnow);
			now = localtime(&rawnow);

			if( (now->tm_wday != 0) && (now->tm_wday != 6)
						&& (now->tm_hour>=9) && (now->tm_hour<17) )
				{
#endif
				if((dbfile = gdbm_open(DBNAME, 512, GDBM_WRITER, S_IRUSR | S_IWUSR, derr1)) == (GDBM_FILE)NULL)
					{
					error("db_auth(): opening database \""DBNAME"\" for write, errno=%d (%s), gdbm_errno=%d",errno,gu_strerror(errno),gdbm_errno);
					ret = USER_ERROR;
					}
				else
					{
					entry_copy->revoked = TRUE;			/* next if will need this */
					time(&entry_copy->last_mod);		/* update last modification time */

					data.dptr = (char*)entry_copy;
					data.dsize = sizeof(struct userdb);

					if(gdbm_store(dbfile, key, data, GDBM_REPLACE))
						{
						error("db_auth(): error storing revised record in database, errno=%d (%s), gdbm_errno=%d", errno, gu_strerror(errno), gdbm_errno);
						ret = USER_ERROR;
						}

					gdbm_close(dbfile);					/* close the database file */
					}
#ifdef BUSINESS_HOURS
				}
#endif
			}
		}

	/* Free the copy of the key converted to lower case. */
	gu_free(key.dptr);

	/* If credit has been revoked, then say the user is overdrawn. */
	if(ret == USER_OK && entry_copy->revoked)
		return USER_OVERDRAWN;
	else
		return ret;
	} /* end of db_auth() */

/* end of file */
