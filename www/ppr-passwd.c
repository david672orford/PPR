/*
** mouse:~ppr/src/www/ppr-passwd.c
** Copyright 1995--2006, Trinity College Computing Center.
** Written by David Chappell.
**
** This file is part of PPR.  You can redistribute it and modify it under the
** terms of the revised BSD licence (without the advertising clause) as
** described in the accompanying file LICENSE.txt.
**
** Last modified 7 September 2006.
*/

/*
** This program updates /etc/ppr/htpasswd, a file which contains MD5 digests
** for use with MD5 Digest HTTP authentication.
*/

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#ifdef INTERNATIONAL
#include <locale.h>
#include <libintl.h>
#endif
#include "gu.h"
#include "gu_md5.h"
#include "global_defines.h"
#include "util_exits.h"
#include "version.h"

const char myname[] = "ppr-passwd";
const char htpasswd[] = CONFDIR"/htpasswd";
const char htpasswd_new[] = CONFDIR"/htpasswd_new";
const char realm[] = "printing";

static int old_password(const char username[], const char realm[], const char digested[]);
static int new_password(FILE *wfile, const char username[], const char realm[]);
static void md5_digest(char *buffer, const char username[], const char realm[], const char password[]);

/*
** Command line options:
*/
static const char *option_chars = "ax";
static const struct gu_getopt_opt option_words[] =
		{
		{"add", 'a', FALSE},
		{"delete", 'x', FALSE},
		{"help", 9000, FALSE},
		{"version", 9001, FALSE},
		{(char*)NULL, 0, FALSE}
		} ;

/*
** Print help.
*/
static void help_usage(FILE *outfile)
	{
	fputs(_("Usage: ppr-passwd <username>\n"), outfile);

	fputc('\n', outfile);

	fputs(_("Valid switches:\n"), outfile);

	fputs(_(	"\t--version\n"
				"\t--help\n"), outfile);
	}

int main(int argc, char *argv[])
	{
	gu_boolean privileged;
	const char *invoking_user;
	const char *username;
	gu_boolean opt_add = FALSE;
	gu_boolean opt_delete = FALSE;
	int rfd, wfd;
	FILE *rfile = NULL, *wfile;

	/* Initialize international messages library. */
	#ifdef INTERNATIONAL
	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
	#endif

	{
	struct passwd *pw;
	uid_t uid = getuid();
	if(!(pw = getpwuid(uid)))
		{
		fprintf(stderr, _("%s: getpwuid(%ld) failed, errno=%d (%s)\n"), myname, (long)getuid(), errno, gu_strerror(errno));
		return EXIT_NOTFOUND;
		}
	invoking_user = gu_strdup(pw->pw_name);

	/* Check if this user is root or ppr or is listed in /etc/ppr/acl/passwd.allow. */
	privileged = user_acl_allows(invoking_user, "passwd");

	/* Prevent user from sending us signals. */
	if(setreuid(geteuid(), -1) == -1)
		{
		fprintf(stderr, "%s: setreuid(%ld, %ld) failed, errno=%d (%s)\n", myname, (long)geteuid(), (long)-1, errno, gu_strerror(errno));
		return EXIT_INTERNAL;
		}
	}

	/* Parse the options. */
	{
	struct gu_getopt_state getopt_state;
	int optchar;
	gu_getopt_init(&getopt_state, argc, argv, option_chars, option_words);
	while((optchar = ppr_getopt(&getopt_state)) != -1)
		{
		switch(optchar)
			{
			case 'a':					/* --add */
				opt_add = TRUE;
				break;
			
			case 'x':					/* --delete */
				opt_delete = TRUE;
				break;

			case 9000:					/* --help */
				help_usage(stdout);
				exit(EXIT_OK);

			case 9001:					/* --version */
				puts(VERSION);
				puts(COPYRIGHT);
				puts(AUTHOR);
				exit(EXIT_OK);

			default:					/* other getopt errors or missing case */
				gu_getopt_default(myname, optchar, &getopt_state, stderr);
				exit(EXIT_SYNTAX);
				break;
			}
		}

	if(argc > getopt_state.optind)
		{
		if(!privileged)
			{
			fprintf(stderr, _("%s: only privileged users may change passwords for others.\n"), myname);
			return EXIT_DENIED;
			}
		username = argv[getopt_state.optind];
		}
	else
		{
		username = invoking_user;
		}
	}

    /* Catching this undefined combination is good policy. */
	if(opt_add && opt_delete)
		{
		fprintf(stderr, _("%s: cannot add and delete a user at the same time.\n"), myname);
		return EXIT_ERROR;
		}

	/*
		Ordinary users aren't allowed to add entries, even for themselves 
		because then someone stepping up to an unattended terminal logged
		into the account of a person who had never used the PPR web interface
		could enable web printer management for that account, setting
		a password known to the interloper.
		
		Note that non-privileged users must know their passwords in order
		to change them.
	*/
	if((opt_add || opt_delete) && !privileged)
		{
		fprintf(stderr, _("%s: only privileged users may add and delete entries.\n"), myname);
		return EXIT_DENIED;
		}

	/* Try to open the existing password file.  If it doesn't exist,
	 * we had better be adding the entry.  If it does exist, lock it
	 * and create a STDIO object.
	 */
	if((rfd = open(htpasswd, O_RDWR)) == -1)
		{
		if(!opt_add)
			{
			fprintf(stderr, _("%s: can't open \"%s\", errno=%d (%s).\n"), myname, htpasswd, errno, gu_strerror(errno));
			return EXIT_INTERNAL;
			}
		}
	else
		{
		if(gu_lock_exclusive(rfd, TRUE) == -1)
			{
			fprintf(stderr, _("%s: failed to get exclusive lock on \"%s\", errno=%d (%s).\n"), myname, htpasswd, errno, gu_strerror(errno));
			return EXIT_INTERNAL;
			}
		if(!(rfile = fdopen(rfd, "r")))
			{
			fprintf(stderr, _("%s: fdopen() failed, errno=%d (%s)\n"), myname, errno, gu_strerror(errno));
			return EXIT_INTERNAL;
			}
		}

	if((wfd = open(htpasswd_new, (O_WRONLY | O_CREAT | O_EXCL), UNIX_640)) == -1)
		{
		fprintf(stderr, _("%s: can't create \"%s\", errno=%d (%s)\n"), myname, htpasswd_new, errno, gu_strerror(errno));
		return EXIT_INTERNAL;
		}

	if(!(wfile = fdopen(wfd, "w")))
		{
		fprintf(stderr, _("%s: fdopen() failed, errno=%d (%s)\n"), myname, errno, gu_strerror(errno));
		return EXIT_INTERNAL;
		}

	{
	int ret = -1;
	{
	char *line = NULL;
	int line_available = 80;
	gu_boolean found = FALSE;
	char *p, *f1, *f2, *f3;
	int linenum = 0;

	/* Step thru all the lines of /etc/ppr/htpasswd. 
	 * Note that the test will fail the first time if
	 * rfile is not open.
	 */
	while(rfile && (p = line = gu_getline(line, &line_available, rfile)))
		{
		linenum++;

		/* Locate the three fields.  Each line looks like this:
				david:printing:c2e93b4cf3b62bdfd5b4680c2a1b8460
		*/
		if(!(f1 = gu_strsep(&p,":")) || !(f2 = gu_strsep(&p,":")) || !(f3 = gu_strsep(&p, ":")))
			{
			fprintf(stderr, _("%s: line %d of %s is invalid, aborting\n"), myname, linenum, htpasswd);
			break;
			}

		/* If this is the one we are looking for, */
		if(strcmp(f1, username) == 0 && strcmp(f2, realm) == 0)
			{
			found = TRUE;

			if(opt_delete)
				{
				ret = 0;
				}
			else
				{
				/* Non-privileged users must prove that they know the old password. */
				if(!privileged)
					{
					if((ret = old_password(username, realm, f3)) != 0)
						break;
					}
				
				/* Get new password from user and write out the user:realm:digest line. */
				if((ret = new_password(wfile, username, realm)) != 0)
					break;
				}
			}

		/* Others just get copied. */
		else
			{
			if(fprintf(wfile, "%s:%s:%s\n", f1, f2, f3) < 0)
				{
				fprintf(stderr, "%s: fprintf() failed\n", myname);
				break;
				}
			}
		}

	/* Line won't have been freed if break in loop. */
	if(line)
		gu_free(line);

	/* done reading /etc/ppr/htpasswd */
	if(rfile && fclose(rfile))
		{
		fprintf(stderr, _("%s: fclose() failed, errno=%d (%s)\n"), myname, errno, gu_strerror(errno));
		ret = -1;
		}

	/* If we haven't already changed an existing password, */
	if(!found)
		{
		if(!opt_add)		/* are we supposed to add if not found? */
			{
			fprintf(stderr, _("%s: user \"%s\" not found in \"%s\".\n"), myname, username, htpasswd);
			ret = EXIT_NOTFOUND;
			}
		else
			{
			ret = new_password(wfile, username, realm);
			}
		}

	}

	/* /etc/ppr/htpasswd_new */
	if(fclose(wfile))
		{
		fprintf(stderr, _("%s: fclose() failed, errno=%d (%s)\n"), myname, errno, gu_strerror(errno));
		ret = -1;
		}

	/* If something above failed, get rid of /etc/ppr/htpasswd_new. */
	if(ret != 0)
		{
		if(unlink(htpasswd_new) == -1)
			{
			fprintf(stderr, _("%s: unlink(\"%s\") failed, errno=%d (%s)\n"), myname, htpasswd_new, errno, gu_strerror(errno));
			}
		printf(_("No changes made.\n"));
		}

	/* Otherwise rename /etc/ppr/htpasswd_new --> /etc/ppr/htpasswd (an atomic operation). */
	else
		{
		if(rename(htpasswd_new, htpasswd) == -1)
			{
			fprintf(stderr, _("%s: rename(\"%s\", \"%s\") failed, errno=%d (%s)\n"), myname, htpasswd_new, htpasswd, errno, gu_strerror(errno));
			ret = -1;
			}
		printf(_("Password file updated sucessfully.\n"));
		}

	if(ret > 0)
		return ret;
	if(ret == -1)
		return EXIT_INTERNAL;
	}

	return EXIT_OK;
	} /* end of main() */

static int old_password(const char username[], const char realm[], const char digested[])
	{
	char prompt[80];
	char *pass_ptr = NULL;
	char old_digested[33];

	snprintf(prompt, sizeof(prompt), _("Please enter old password for user %s in realm %s: "), username, realm);

	while(!pass_ptr)
		{
		if(!(pass_ptr = getpass(prompt)))
			return -1;
		if(strlen(pass_ptr) == 0)
			return EXIT_USER_ABORT;
		}

	md5_digest(old_digested, username, realm, pass_ptr);
	memset(pass_ptr, 0, strlen(pass_ptr));

	if(strcmp(old_digested, digested) != 0)
		{
		printf(_("Password incorrect, access denied.\n"));
		return EXIT_DENIED;
		}

	return 0;
	}

static int new_password(FILE *wfile, const char username[], const char realm[])
	{
	char prompt[80];
	char *pass_ptr_first = NULL;
	char *pass_ptr = NULL;
	char crypted[33];
	gu_boolean match;

	snprintf(prompt, sizeof(prompt), _("Please enter new password for user %s in realm %s: "), username, realm);

	do	{
		while(!pass_ptr)
			{
			if(!(pass_ptr = getpass(prompt)))
				return -1;
			if(strlen(pass_ptr) == 0)
				return EXIT_USER_ABORT;
			if(strlen(pass_ptr) < 6)
				{
				memset(pass_ptr, 0, strlen(pass_ptr));
				printf(_("That password is too short.  Please pick another.\n"));
				pass_ptr = NULL;
				continue;
				}
			break;
			}

		pass_ptr_first = gu_strdup(pass_ptr);
		memset(pass_ptr, 0, strlen(pass_ptr));

		pass_ptr = NULL;
		while(!pass_ptr)
			{
			if(!(pass_ptr = getpass(_("Please verify new password: "))))
				break;
			}

		match = (pass_ptr && strcmp(pass_ptr_first, pass_ptr) == 0);
		memset(pass_ptr_first, 0, strlen(pass_ptr_first));
		gu_free(pass_ptr_first);
		pass_ptr_first = NULL;

		if(!match)
			{
			memset(pass_ptr, 0, strlen(pass_ptr));
			fputc('\n', stdout);
			printf(_("Passwords do not match, please try again.\n"));
			pass_ptr = NULL;
			}
		} while(!match);

	/* Create the MD5 signiture and then zero the cleartext password out
	   to keep it from prying eyes.
	   */
	md5_digest(crypted, username, realm, pass_ptr);
	memset(pass_ptr, 0, strlen(pass_ptr));

	if(fprintf(wfile, "%s:%s:%s\n", username, realm, crypted) < 0)
		{
		fprintf(stderr, "%s: fprintf() failed\n", myname);
		return -1;
		}

	return 0;
	}

/* Make an MD5 digest of username:realm:password and store it as a 32 byte 
 * hexadecimal string in buffer[] (which means that buffer[] must be 33
 * bytes long.
 */
static void md5_digest(char *buffer, const char username[], const char realm[], const char password[])
	{
	md5_state_t state;
	md5_byte_t digest[16];
	int i;

	md5_init(&state);
	md5_append(&state, (const md5_byte_t *)username, strlen(username));
	md5_append(&state, (const md5_byte_t *)":", strlen(":"));
	md5_append(&state, (const md5_byte_t *)realm, strlen(realm));
	md5_append(&state, (const md5_byte_t *)":", strlen(":"));
	md5_append(&state, (const md5_byte_t *)password, strlen(password));
	md5_finish(&state, digest);

	for(i = 0; i < 16; i++)
		snprintf(&buffer[i * 2], 3, "%02x", digest[i]);
	}

/* end of file */
