/*
** mouse:~ppr/src/libpprdb/dbmod.c
** Copyright 1995--2000, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Last modified 9 September 2000.
*/

/*
** Make modifications to the user database.
*/

#include "before_system.h"
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <gdbm.h>				/* gnu database library */
#include <errno.h>
#include "gu.h"
#include "global_defines.h"

#include "userdb.h"

extern gdbm_error gdbm_errno;	/* needed for gdbm 1.5 */

/*
** derr(), a dummy error handler for gdbm
*/
#ifdef __cplusplus
extern "C" void derr(...);
void derr(...) { }
#else
static void derr(char *str) { }
#endif

/*
** Add a user to the database.	Return non-zero if
** the operation fails for any reason.
*/
enum USERDB_RESULT db_add_user(const char *username, struct userdb *user)
	{
	GDBM_FILE dbfile;
	datum key;
	datum data;
	enum USERDB_RESULT ret = USER_OK;

	if((dbfile = gdbm_open(DBNAME,0,GDBM_WRCREAT,S_IRUSR|S_IWUSR,derr)) == (GDBM_FILE)NULL)
		{
		error("db_add_user(): error opening database \""DBNAME"\", errno=%d (%s), gdbm_errno=%d",errno,gu_strerror(errno),gdbm_errno);
		return USER_ERROR;
		}

	key.dptr = dbstrlower(username);	/* fill in key datum with pointer */
	key.dsize = strlen(username);		/* to key (user name) and length */

	data.dptr = (char*)user;			/* do the same for the data datum */
	data.dsize = sizeof(struct userdb);

	time(&user->last_mod);				/* last modification is now */

	{
	int gdbm_ret = gdbm_store(dbfile, key, data, GDBM_INSERT);
	if(gdbm_ret == 1)
		{
		error("The entry already exists.");
		ret = USER_ERROR;
		}
	else if(gdbm_ret != 0)
		{
		error("db_add_user(): error, errno=%d (%s), gdbm_errno=%d", errno, gu_strerror(errno), gdbm_errno);
		ret = USER_ERROR;
		}
	}

	gdbm_close(dbfile);
	gu_free(key.dptr);					 /* deallocate dbstrlower() block */
	return ret;
	} /* end of db_add_user() */

/*
** Delete a user from the database.	 Return non-zero if
** the operation fails for any reason.
*/
enum USERDB_RESULT db_delete_user(const char *username)
	{
	GDBM_FILE dbfile;
	datum key;
	enum USERDB_RESULT ret = USER_OK;

	key.dptr = dbstrlower(username);
	key.dsize = strlen(username);

	if((dbfile = gdbm_open(DBNAME,0,GDBM_WRITER,S_IRUSR|S_IWUSR,derr)) == (GDBM_FILE)NULL)
		{
		error("db_delete_user(): error opening database \""DBNAME"\", errno=%d (%s), gdbm_errno=%d",errno,gu_strerror(errno),gdbm_errno);
		return USER_ERROR;
		}

	if(gdbm_delete(dbfile, key))
		ret = USER_ERROR;

	gdbm_close(dbfile);
	gu_free(key.dptr);			/* free memory allocated by dbstrlower() */
	return ret;
	} /* en dof db_delete_user() */

/*
** Change a user's authcode.
*/
enum USERDB_RESULT db_new_authcode(const char *username, const char *newauthcode)
	{
	GDBM_FILE dbfile;
	datum key;
	datum data;
	struct userdb *entry;
	enum USERDB_RESULT ret = USER_OK;

	key.dptr = dbstrlower(username);
	key.dsize = strlen(username);

	if((dbfile = gdbm_open(DBNAME,0,GDBM_WRITER,S_IRUSR|S_IWUSR,derr)) == (GDBM_FILE)NULL)
		{
		error("db_new_authcode(): error opening database \""DBNAME"\", errno=%d (%s), gdbm_errno=%d",errno,gu_strerror(errno),gdbm_errno);
		return USER_ERROR;					/* can't open is database error */
		}

	data = gdbm_fetch(dbfile, key);		/* look up user */
	if(data.dptr == (char*)NULL)		/* if data pointer is null, */
		{
		ret = USER_ISNT;				/* user wasn't found */
		}
	else
		{
		entry = (struct userdb*)data.dptr;

		strncpy(entry->authcode, newauthcode, 16);		/* the AuthCode */
		entry->authcode[16] = (char)NULL;

		time(&entry->last_mod);			/* update last modification time */

		if(gdbm_store(dbfile, key, data, GDBM_REPLACE))
			{
			error("db_new_authcode(): error storing in database, errno=%d (%s), gdbm_errno=%d",errno,gu_strerror(errno),gdbm_errno);
			ret = USER_ERROR;
			}
		free(data.dptr);				/* gdbm make this, so I use free()? */
		}

	gu_free(key.dptr);					/* free memory allocated by dbstrlower() */
	gdbm_close(dbfile);
	return ret;
	} /* end of db_new_authcode() */

/* end of file */
