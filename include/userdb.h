/*
** mouse:~ppr/src/include/userdb.h
** Copyright 1995, 1996, 1997, 1998, Trinity College Computing Center.
** Writen by David Chappell.
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
** The is a header file for modules which use the user account charge
** routines.  There are no configuration parameters in this file.
*/

#include <time.h>

enum USERDB_RESULT {
		USER_ERROR = -2,		/* database error */
		USER_ISNT = -1,			/* user not found */
		USER_OK = 0,			/* user exists, action sucessful */
		USER_OVERDRAWN = 1		/* user exists and is overdrawn */
		} ;

enum TRANSACTION {
		TRANSACTION_CHARGE = 0,
		TRANSACTION_DEPOSIT = 1,
		TRANSACTION_WITHDRAWAL = 2,
		TRANSACTION_CORRECTION = 3
		} ;

#define MAX_AUTHCODE 16
#define MAX_FULLNAME 32

struct userdb
	{
	char authcode[MAX_AUTHCODE+1];		/* 16 characters and one null */
	char fullname[MAX_FULLNAME+1];		/* 32 characters and one null */
	int balance;			/* balance x 100 */
	int cutoff;				/* credit cutoff point x 100 */
	char revoked;			/* TRUE or FALSE, credit revoked */
	time_t last_mod;		/* date of last modification */
	int lifetime;			/* days of inactivity allowed before delete */
	} ;

char *dbstrlower(const char *s);
enum USERDB_RESULT db_auth(struct userdb *user, const char *username);
enum USERDB_RESULT db_add_user(const char *username, struct userdb *user);
enum USERDB_RESULT db_delete_user(const char *username);
enum USERDB_RESULT db_transaction(const char *username, int amount, enum TRANSACTION transaction_type);
enum USERDB_RESULT db_new_authcode(const char *username, const char *newauthcode);

/* end of file */

