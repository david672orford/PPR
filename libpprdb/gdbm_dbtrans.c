/*
** mouse:~ppr/src/libpprdb/dbtrans.c
** Copyright 1995, 1996, 1997, 1998, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Last modified 13 October 1998.
*/

/*
** Make a printing charge to a user's database entry.
** Also, make a deposit or a correction.
*/

#include "before_system.h"
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <gdbm.h>
#include "gu.h"
#include "global_defines.h"

#include "userdb.h"

extern gdbm_error gdbm_errno;	/* needed for gdbm 1.5 */

/*
** derr(), a dummy error handler for gdbm
*/
#ifdef __cplusplus
extern "C" void derr2(...);
void derr2(...) { }
#else
static void derr2(char *s) { }
#endif

enum USERDB_RESULT db_transaction(const char *username, int amount, enum TRANSACTION transaction_type)
    {
    GDBM_FILE dbfile;
    datum key;
    datum data;
    enum USERDB_RESULT ret = USER_OK;

    if((dbfile = gdbm_open(DBNAME, 0, GDBM_WRITER, S_IRUSR | S_IWUSR, derr2)) == (GDBM_FILE)NULL)
	{
	error("db_transaction(): error opening database \""DBNAME"\", errno=%d (%s), gdbm_errno=%d", errno, gu_strerror(errno), gdbm_errno);
	return USER_ERROR;                  /* can't open is database error */
	}

    /*
    ** The key used for the lookup will be a lower case
    ** version of the user name.  It will be created in
    ** malloc()ed memory.
    */
    key.dptr = dbstrlower(username);
    key.dsize = strlen(username);

    data = gdbm_fetch(dbfile, key);	/* look up user */
    if(data.dptr == (char*)NULL)	/* if data pointer is null, */
	{
	ret = USER_ISNT;		/* user wasn't found */
	}
    else
	{
	struct userdb *entry = (struct userdb*)data.dptr;

	switch(transaction_type)	/* transaction type makes a */
	    {				/* difference, it determines if */
	    case TRANSACTION_CHARGE:	/* we add or subtract */
	    case TRANSACTION_WITHDRAWAL:
		entry->balance -= amount;
		break;
	    case TRANSACTION_DEPOSIT:	/* any deposit */
		entry->revoked = FALSE;	/* restores credit and falls thru */
	    case TRANSACTION_CORRECTION:
		entry->balance += amount;
		break;
	    default:
		error("db_transaction(): invalid transaction type=%d, amount=%d\n", transaction_type, amount);
		ret = USER_ERROR;
		break;
	    }

	time(&entry->last_mod);		/* update last modification time */

	if(gdbm_store(dbfile, key, data, GDBM_REPLACE))
	    {
	    error("db_transaction(): error storing in database, errno=%d (%s), gdbm_errno=%d", errno, gu_strerror(errno), gdbm_errno);
	    ret = USER_ERROR;
	    }

	free(data.dptr);		/* free memory allocated by gdbm_fetch() */
	}

    gu_free(key.dptr);			/* free memory allocated by dbstrlower() */
    gdbm_close(dbfile);
    return USER_OK;
    } /* end of db_transaction() */

/* end of file */
