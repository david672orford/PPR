/*
** mouse:~ppr/src/libpprdb/dbmod.c
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
** Dummies for functions to make modifications to the user database.
*/

#include "before_system.h"
#include "gu.h"
#include "global_defines.h"

#include "userdb.h"

/*
** Add a user to the database.	Return non-zero if
** the operation fails for any reason.
*/
enum USERDB_RESULT db_add_user(const char *username, struct userdb *user)
	{
	error("db_add_user(): user database support not compiled in");
	return USER_ERROR;
	} /* end of db_add_user() */

/*
** Delete a user from the database.	 Return non-zero if
** the operation fails for any reason.
*/
enum USERDB_RESULT db_delete_user(const char *username)
	{
	error("db_delete_user(): user database support not compiled in");
	return USER_ERROR;
	} /* en dof db_delete_user() */

/*
** Change a user's authcode.
*/
enum USERDB_RESULT db_new_authcode(const char *username, const char *newauthcode)
	{
	error("db_new_authcode(): user database support not compiled in");
	return USER_ERROR;
	} /* end of db_new_authcode() */

/* end of file */
