/*
** mouse:~ppr/src/ppuser/ppuser.c
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
** Last modified 3 April 2006.
*/

/*
** This is the operator's user account manipulation program.
*/

#include "config.h"
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
#include <ctype.h>
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

/*
** Parse a float, first trying in the user's locale, and then,
** if that doesn't work, we try to parse a float in the notation
** of the C locale.
*/
static int parse_float(const char string[], float *store_here)
	{
	char *stop_ptr;

	*store_here = (float)strtod(string, (char**)&stop_ptr);

	if(!*stop_ptr && isspace(*stop_ptr))
		return 1;

	return gu_sscanf(string, "%f", store_here);
	}

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
			"\tppuser add <username> '<real name>' <authcode> <initial balance> <cutoff> <life>\n"), stderr);
		return EXIT_SYNTAX;
		}

	username = argv[0];

	strncpy(data.fullname, argv[1], MAX_FULLNAME);		/* the real name */
	data.fullname[MAX_FULLNAME] = '\0';

	strncpy(data.authcode, argv[2], MAX_AUTHCODE);		/* the AuthCode */
	data.authcode[MAX_AUTHCODE] = '\0';

	parse_float(argv[3], &x);					/* the initial balance */
	x *= 100.0;
	data.balance = (int)x;

	parse_float(argv[4], &x);					/* the credit cutoff point */
	x *= 100.0;
	data.cutoff = (int)x;

	gu_sscanf(argv[5], "%d", &data.lifetime);	/* how long to preserve */

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
** Show a user's recored.
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
			char temp[64];
			t = localtime(&entry.last_mod);
			strftime(temp, sizeof(temp), "%c", t);
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

	parse_float(argv[1], &x);
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
		"\tppuser add <username> '<real name>' <authcode> <initial balance> <cutoff> <life>\n"
		"\tppuser delete <username>\n"
		"\tppuser show <username>\n"
		"\tppuser deposit <username> amount\n"
		"\tppuser withdraw <username> amount\n"
		"\tppuser charge <username> amount\n"
		"\tppuser correction <username> amount\n"), outfile);
	} /* end of help */

/*
** main() function and command dispatcher
*/
int main(int argc, char *argv[])
	{
	/* Initialize internation messages library. */
	#ifdef INTERNATIONAL
	setlocale(LC_ALL, "");
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

