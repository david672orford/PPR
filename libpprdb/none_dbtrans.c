/*
** mouse:~ppr/src/libpprdb/none_dbtrans.c
** Copyright 1995--1999, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Last modified 1 July 1999.
*/

/*
** Dummies for functions to make a printing charge to
** a user's database entry.  Also, make a deposit or
** a correction.
*/

#include "config.h"
#include "gu.h"
#include "global_defines.h"

#include "userdb.h"

enum USERDB_RESULT db_transaction(const char *username, int amount, enum TRANSACTION transaction_type)
	{
	error("db_transaction(): user database support not compiled in");
	return USER_ERROR;
	} /* end of db_transaction() */

/* end of file */

