/*
** mouse:~ppr/src/libpprdb/dbauth.c
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
** Authorization function, return the user's record and indicate if printing
** can be carried out in consideration of the amount of money currently
** in the account.
*/

#include "config.h"
#include "gu.h"
#include "global_defines.h"

#include "userdb.h"

enum USERDB_RESULT db_auth(struct userdb *entry_copy, const char *username)
	{
	error("db_auth(): user database support not compiled in");
	return USER_ERROR;
	} /* end of db_auth() */

/* end of file */

