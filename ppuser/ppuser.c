/*
** mouse:~ppr/src/ppuser/ppuser.c
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
** Last modified 29 May 2001.
*/

/*
** This is the operator's user account manipulation program.
*/

#include "before_system.h"
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <stdarg.h>
#include <pwd.h>
#include <errno.h>
#include <fcntl.h>
#ifdef INTERNATIONAL
#include <locale.h>
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "userdb.h"
#include "util_exits.h"

/* File where we log transactions: */
#define PPUSER_LOGFILE LOGDIR"/ppuser"

/*
** Handle non-fatal errors.
** This is called from functions in libpprdb.
*/
void error(const char *message, ... )
	{
	va_list va;

	va_start(va, message);
	vfprintf(stderr, message, va);
	fprintf(stderr, "\n");
	va_end(va);
	} /* end of error() */

/*
** Function for logging ppuser commands.  The log will be a valid
** shell script playing back the commands.
**
** This code was contributed by Klaus vom Scheidt
** <vscheidt@sun10rz2.rz.uni-leipzig.de>.
**
** called for:	ppuser delete ...
**				ppuser authcode ...
**				ppuser deposit ...
**				ppuser withdraw ..
**				ppuser charge ...
**				ppuser correction ...
**
** The log is written to the file LOGFILE"/ppuser",
** if that file exists.
*/
static void log_ppuser(const char *fmt, ...)
	{
	va_list va;
	FILE *file_ppuser;
	int file_ppuser_fd;

	if((file_ppuser_fd = open(PPUSER_LOGFILE, O_WRONLY | O_APPEND)) >= 0)
		{
		if((file_ppuser = fdopen(file_ppuser_fd, "a")) == NULL)
			{
			fprintf(stderr, "log_ppuser(): fdopen() failed");
			close(file_ppuser_fd);
			}
		else
			{
			va_start(va, fmt);
			fprintf(file_ppuser, "# %s \n", datestamp() );
			vfprintf(file_ppuser, fmt, va);
			va_end(va);
			fclose(file_ppuser);
			}
		}
	} /* end of log_ppuser() */

/*
** Return true if the user running this program is one that is
** allowed to perform privledged operations such as making
** deposits and posting charges.
*/
static gu_boolean privledged(void)
	{
	static int answer = -1;		/* -1 means undetermined */

	if(answer == -1)			/* if undetermined */
		{
		uid_t uid = getuid();
		answer = 0;				/* Start with "false". */

		/* Of course, "ppr" is privledged, as is uid 0 (root). */
		if(uid == 0 || uid == geteuid())
			{
			answer = 1;			/* Change answer to "true". */
			}

		/* As are all users who are members of the group "ppr". */
		else
			{
			struct passwd *pw;

			if((pw = getpwuid(uid)) == (struct passwd *)NULL)
				{
				fprintf(stderr, "privledged(): getpwuid() failed, errno=%d (%s)\n", errno, gu_strerror(errno));
				exit(EXIT_INTERNAL);
				}

			if(user_acl_allows(pw->pw_name, "ppuser"))
				{
				answer = 1;		/* Change to "true". */
				}
			}
		} /* end of if answer not determined yet */

	return answer ? TRUE : FALSE;
	} /* end of privledged() */

/*
** Return non-zero if the user is a PPR user charge
** account administrator.  Also, print an error message.
*/
static int am_administrator(void)
	{
	if( privledged() )
		{
		return TRUE;
		}
	else
		{
		fputs(_("You are not allowed to perform the requested operation because\n"
				"you are not a PPR user charge account administrator.\n"), stderr);
		return FALSE;
		}
	} /* end of am_administrator() */

/*===========================================================================
** Command action routines
===========================================================================*/

/*
** Add a record to the database.
*/
static int ppuser_add(char *argv[])
	{
	const char *username;
	struct userdb data;
	float x;

	if(! am_administrator() )
		return EXIT_DENIED;

	if(! argv[0] || ! argv[1] || ! argv[2] || ! argv[3] || ! argv[4] || ! argv[5])
		{
		fputs(_("Usage:\n"
			"\tppuser add '<user name>' '<real name>' <authcode> <initial balance> <cutoff> <life>\n"), stderr);
		return EXIT_SYNTAX;
		}

	username = argv[0];

	strncpy(data.fullname,argv[1], MAX_FULLNAME);		/* the real name */
	data.fullname[MAX_FULLNAME] = '\0';

	strncpy(data.authcode,argv[2], MAX_AUTHCODE);		/* the AuthCode */
	data.authcode[MAX_AUTHCODE] = '\0';

	sscanf(argv[3],"%f",&x);					/* the initial balance */
	x *= 100.0;
	data.balance = (int)x;

	sscanf(argv[4],"%f",&x);					/* the credit cutoff point */
	x *= 100.0;
	data.cutoff = (int)x;

	sscanf(argv[5], "%d", &data.lifetime);		/* how long to preserve */

	data.revoked = FALSE;						/* credit not revoked */

	switch(db_add_user(username, &data))
		{
		case USER_OK:
			log_ppuser("ppuser add \"%s\" \"%s\" %s %s %s %s\n",
				argv[0], argv[1], argv[2], argv[3], argv[4], argv[5]);
			return EXIT_OK;
		default:
			fprintf(stderr, _("Database error.\n"));
			return EXIT_INTERNAL;
		}
	} /* end of ppuser_add() */

/*
** Delete a record from the database.
*/
static int ppuser_delete(char *argv[])
	{
	const char *username;

	if(! am_administrator() )
		return EXIT_DENIED;

	if(! (username = argv[0]))
		{
		fprintf(stderr, _("Usage:\n\tppuser delete '<user name>'\n"));
		return EXIT_SYNTAX;
		}

	switch(db_delete_user(username))
		{
		case USER_ISNT:
			fprintf(stderr, _("The user \"%s\" does not exist.\n"), username);
			return EXIT_NOTFOUND;
		case USER_OK:
			log_ppuser("ppuser delete \"%s\"\n", username);
			return EXIT_OK;
		default:
			fprintf(stderr, _("Database error.\n"));
			return EXIT_INTERNAL;
		}
	} /* end of ppuser_delete() */

/*
** Show a user's recored, all except the authcode field.
*/
static int ppuser_show(char *argv[])
	{
	struct userdb entry;
	const char *username = argv[0];

	if(! username)
		{
		fprintf(stderr, _("Usage:\n\tppuser show '<user name>'\n"));
		return EXIT_SYNTAX;
		}

	switch(db_auth(&entry, username))
		{
		case USER_OK:
		case USER_OVERDRAWN:
			printf(_("Name: %s\n"), username);
			printf(_("Fullname: %s\n"), entry.fullname);
			printf(_("Balance: %s\n"), money(entry.balance));
			printf(_("Cutoff point: %s\n"), money(entry.cutoff));
			{
			struct tm *t;
			char temp[32];
			t = localtime(&entry.last_mod);
			strftime(temp, sizeof(temp), "%d %b %Y %I:%M %p", t);
			printf(_("Last Modified: %s\n"), temp);
			}
			printf(_("Account lifetime: %d\n"), entry.lifetime);
			printf(_("Credit revoked: %s\n"), entry.revoked ? _("TRUE") : _("FALSE"));
			return EXIT_OK;
		case USER_ISNT:
			printf(_("The user \"%s\" does not exist.\n"), username);
			return EXIT_NOTFOUND;
		default:
			fprintf(stderr, _("Database error.\n"));
			return EXIT_INTERNAL;
		}
	} /* end of ppuser_show() */

/*
** Change a user's authcode.
*/
static int ppuser_authcode(char *argv[])
	{
	const char *username, *newauthcode;

	if(! am_administrator() )
		return EXIT_DENIED;

	username = argv[0];
	newauthcode = argv[1];

	if(! username || ! newauthcode)
		{
		fprintf(stderr, _("Usage: ppuser authcode '<UserName>' <NewAuthCode>\n"));
		return EXIT_SYNTAX;
		}

	switch(db_new_authcode(username,newauthcode))
		{
		case USER_ISNT:
			fprintf(stderr, _("The user \"%s\" does not exist.\n"), username);
			return EXIT_NOTFOUND;
		case USER_OK:
			log_ppuser("ppuser authcode \"%s\" \"%s\"\n", username, newauthcode);
			return EXIT_OK;
		default:
			fprintf(stderr, _("Database error.\n"));
			return EXIT_INTERNAL;
		}
	} /* ppuser_authcode() */

/*
** Deposit money in a users account.
*/
static int ppuser_transaction(char *argv[], enum TRANSACTION transaction_type)
	{
	const char *username;
	float x;
	int amount;
	int ret;

	if(! am_administrator() )
		return EXIT_DENIED;

	if(! (username = argv[0]) || ! argv[1])
		{
		fputs(_("Usage:\n"
			"\tppuser {deposit,withdraw,charge,correction} '<user name>' <amount>\n"), stderr);
		return EXIT_SYNTAX;
		}

	sscanf(argv[1],"%f",&x);
	x *= 100.0;
	amount = (int)x;

	ret = db_transaction(username, amount, transaction_type);

	switch(ret)
		{
		case USER_OK:
			switch(transaction_type)
				{
				case TRANSACTION_CHARGE:
					log_ppuser("ppuser charge \"%s\" %s\n", username, argv[1]);
					break;
				case TRANSACTION_DEPOSIT:
					log_ppuser("ppuser deposit \"%s\" %s\n", username, argv[1]);
					break;
				case TRANSACTION_WITHDRAWAL:
					log_ppuser("ppuser withdraw \"%s\" %s\n", username, argv[1]);
					break;
				case TRANSACTION_CORRECTION:
					log_ppuser("ppuser correction \"%s\" %s\n", username, argv[1]);
					break;
				}
			return EXIT_OK;
		case USER_ISNT:
			fprintf(stderr, _("The user \"%s\" does not exist.\n"), username);
			return EXIT_NOTFOUND;
		default:
			fprintf(stderr, _("Database error, transaction not processed.\n"));
			return EXIT_INTERNAL;
		}
	} /* end of ppuser_transaction() */

/*
** called from main() if no valid command
*/
static void help(FILE *outfile)
	{
	fputs(_("Usage:\n"
		"\tppuser add '<user name>' '<real name>' <authcode> <initial balance> <cutoff> <life>\n"
		"\tppuser delete '<user name>'\n"
		"\tppuser show '<user name>'\n"
		"\tppuser authcode '<user name>' <authcode>\n"
		"\tppuser deposit '<user name>' amount\n"
		"\tppuser withdraw '<user name>' amount\n"
		"\tppuser charge '<user name>' amount\n"
		"\tppuser correction '<user name>' amount\n"), outfile);
	} /* end of help */

/*
** main() function and command dispatcher
*/
int main(int argc, char *argv[])
	{
	/* Initialize internation messages library. */
	#ifdef INTERNATIONAL
	setlocale(LC_MESSAGES, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
	#endif

	/* parnoia */
	umask(PPR_UMASK);

	if(argc < 2)		/* if too few to be any valid command */
		{
		help(stderr);
		return EXIT_SYNTAX;
		}

	if(gu_strcasecmp(argv[1], "add") == 0)
		return ppuser_add(&argv[2]);
	else if(gu_strcasecmp(argv[1], "delete") == 0)
		return ppuser_delete(&argv[2]);
	else if(gu_strcasecmp(argv[1], "show") == 0)
		return ppuser_show(&argv[2]);
	else if(gu_strcasecmp(argv[1], "authcode") == 0)
		return ppuser_authcode(&argv[2]);
	else if(gu_strcasecmp(argv[1], "deposit") == 0)
		return ppuser_transaction(&argv[2],TRANSACTION_DEPOSIT);
	else if(gu_strcasecmp(argv[1], "withdraw") == 0)
		return ppuser_transaction(&argv[2],TRANSACTION_WITHDRAWAL);
	else if(gu_strcasecmp(argv[1], "charge") == 0)
		return ppuser_transaction(&argv[2],TRANSACTION_CHARGE);
	else if(gu_strcasecmp(argv[1], "correction") == 0)
		return ppuser_transaction(&argv[2],TRANSACTION_CORRECTION);
	else if(gu_strcasecmp(argv[1], "help") == 0)
		{
		help(stdout);
		return EXIT_OK;
		}
	else
		{
		help(stderr);
		return EXIT_SYNTAX;
		}
	} /* end of main() */

/* end of file */

