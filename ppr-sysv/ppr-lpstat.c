/*
** mouse:~ppr/src/ppr-sysv/ppr-lpstat.c
** Copyright 1995--2005, Trinity College Computing Center.
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
** Last modified 14 January 2005.
*/

#include "config.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <pwd.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#ifdef INTERNATIONAL
#include <locale.h>
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "uprint.h"

/*
** This program is a wrapper that emulates lpstat from System V Unix.
**
** It needs work!
*/

/* This name will appear in certain error messages: */
static const char myname[] = "ppr-lpstat";

void uprint_error_callback(const char *format, ...)
	{
	va_list va;
	fprintf(stderr, "%s: ", myname);
	va_start(va, format);
	vfprintf(stderr, format, va);
	fputc('\n', stderr);
	va_end(va);
	} /* end of uprint_error_callback() */

/*

This function implementations "lpstat -a" (which indicate job acceptance
status of printers).  Here is a sample command and sample output:

$ real-lpstat -a x,y,z,dummy
x: unknown printer
y: unknown printer
z: unknown printer
dummy accepting requests since Apr 17 12:12 2002

*/
static void lpstat_a(const char list[])
	{
	/*
	** Chug through the uprint-remote.conf file and print dummy lines for the
	** queues.  We don't have real acceptance times, so we print a fake time.
	*/
	{
	FILE *f;
	if((f = fopen(UPRINTREMOTECONF, "r")) != (FILE*)NULL)
		{
		char *line = NULL;
		int line_available = 80;
		while((line = gu_getline(line, &line_available, f)))
			{
			if(line[0] == '[')
				{
				line[strcspn(line, "]")] = '\0';
				if(!strchr(line+1, '?') && !strchr(line+1, '*'))
					printf("%s accepting requests since Jan 1 00:00 1970\n", line + 1);
				}
			}
		fclose(f);
		}
	}

	/*
	** Chug through the PPR printers, printing dummy acceptance messages as
	** (again with fake times) we go.
	*/
	{
	DIR *dir;
	struct dirent *direntp;
	int len;
	if((dir = opendir(PRCONF)))
		{
		while((direntp = readdir(dir)))
			{
			/* Skip . and .. and hidden files. */
			if(direntp->d_name[0] == '.')
				continue;

			/* Skip Emacs style backup files. */
			len = strlen(direntp->d_name);
			if( len > 0 && direntp->d_name[len-1]=='~' )
				continue;

			printf("%s accepting requests since Jan 1 00:00 1970\n", direntp->d_name);
			}
		}
	}

	} /* end of lpstat_a() */

/*

This function implementations "lpstat -c" (which lists classes and their
members).  Here is are some sample commands and sample output:

$ real-lpstat -c
members of class testclass:
		testprn

$ real-lpstat -c x,y
UX:lpstat: ERROR: Class "x" does not exist.
		  TO FIX: Use the "lpstat -c all" command to list
				  all known classes.
UX:lpstat: ERROR: Class "y" does not exist.
		  TO FIX: Use the "lpstat -c all" command to list
				  all known classes.

*/
static void lpstat_c(const char list[])
	{
	/*
	** Chug through the PPR groups.
	*/
	{
	DIR *dir;
	struct dirent *direntp;
	int len;
	if((dir = opendir(GRCONF)))
		{
		while((direntp = readdir(dir)))
			{
			/* Skip . and .. and hidden files. */
			if(direntp->d_name[0] == '.')
				continue;

			/* Skip Emacs style backup files. */
			len = strlen(direntp->d_name);
			if( len > 0 && direntp->d_name[len-1]=='~' )
				continue;

			printf("members of class %s:\n", direntp->d_name);
			}
		}
	}

	}

/*

This function implementations "lpstat -o" (which lists jobs).  Here is a
sample command and sample output:

$ real-lpstat -o x,y,z,dummy
x: unknown printer
y: unknown printer
z: unknown printer

$ /usr/lib/lp/local/lpstat -o testprn
testprn-109				root			   270	 Jul 19 14:24
testprn-110				root			   270	 Jul 19 14:25

*/
static void lpstat_o(const char list[])
	{
	}

/*

This function implementations "lpstat -p" (which displays the operational 
status of printers).  Here is are some sample commands and sample output from
Solaris 2.6.  Note that Solaris 2.6 wraps the lpstat command just like UPRINT 
does.  The wrapper is in /usr/bin/lpstat, the real (System V release 4.0) one
is in /usr/lib/lp/local/lpstat.  The wrapper provides remote printing which
System V release 4.0 lp does not.  Note the differences in the output.

$ /usr/bin/lpstat -p x,dummy
x: unknown printer
printer dummy unknown state. enabled since Jul 19 13:01 2002. available.

$ /usr/bin/lpstat -p
printer dummy unknown state. enabled since Jul 19 14:31 2002. available.
printer testprn disabled since Fri Jul 19 14:05:15 EDT 2002. available.
		new printer
printer testclass faulted printing testclass-0. enabled since Jul 19 14:32 2002. available.
		server shakti not responding
printer usrgeni faulted printing usrgeni-54. enabled since Jul 19 14:32 2002. available.
		server ads.cc.trincoll.edu not responding

$ /usr/bin/lpstat -p -l
printer dummy unknown state. enabled since Jul 19 14:35 2002. available.
		Remote Name: dummy
		Remote Server: mouse.cc.trincoll.edu
printer testprn disabled since Fri Jul 19 14:05:15 EDT 2002. available.
		new printer
		Form mounted:
		Content types: simple
		Printer types: unknown
		Description:
		Connection: direct
		Interface: /usr/lib/lp/model/standard
		On fault: write to root once
		After fault: continue
		Users allowed:
				(all)
		Forms allowed:
				(none)
		Banner required
		Character sets:
				(none)
		Default pitch:
		Default page size:
		Default port settings:

printer testclass faulted printing testclass-0. enabled since Jul 19 14:36 2002. available.
		server shakti not responding
		Remote Name: testclass
		Remote Server: shakti
printer usrgeni faulted printing usrgeni-54. enabled since Jul 19 14:36 2002. available.
		server ads.cc.trincoll.edu not responding
		Remote Name: UPSTAIRS$GENICOM
		Remote Server: ads.cc.trincoll.edu

$ /usr/bin/lpstat -p -D
printer dummy unknown state. enabled since Jul 19 14:41 2002. available.
		Description: dummy@mouse.cc.trincoll.edu
printer testprn disabled since Fri Jul 19 14:05:15 EDT 2002. available.
		new printer
		Description:
printer testclass faulted printing testclass-0. enabled since Jul 19 14:42 2002. available.
		server shakti not responding
		Description: testclass@shakti
printer usrgeni faulted printing usrgeni-54. enabled since Jul 19 14:42 2002. available.
		server ads.cc.trincoll.edu not responding
		Description: UPSTAIRS$GENICOM@ads.cc.trincoll.edu

$ /usr/bin/lpstat -p -l -D
printer dummy unknown state. enabled since Jul 19 14:38 2002. available.
		Description: dummy@mouse.cc.trincoll.edu
		Remote Name: dummy
		Remote Server: mouse.cc.trincoll.edu
printer testprn disabled since Fri Jul 19 14:05:15 EDT 2002. available.
		new printer
		Form mounted:
		Content types: simple
		Printer types: unknown
		Description:
		Connection: direct
		Interface: /usr/lib/lp/model/standard
		On fault: write to root once
		After fault: continue
		Users allowed:
				(all)
		Forms allowed:
				(none)
		Banner required
		Character sets:
				(none)
		Default pitch:
		Default page size:
		Default port settings:

printer testclass faulted printing testclass-0. enabled since Jul 19 14:39 2002. available.
		server shakti not responding
		Description: testclass@shakti
		Remote Name: testclass
		Remote Server: shakti
printer usrgeni faulted printing usrgeni-54. enabled since Jul 19 14:39 2002. available.
		server ads.cc.trincoll.edu not responding
		Description: UPSTAIRS$GENICOM@ads.cc.trincoll.edu
		Remote Name: UPSTAIRS$GENICOM
		Remote Server: ads.cc.trincoll.edu

$ /usr/lib/lp/local/lpstat -p x,dummy
UX:lpstat: ERROR: Printer "x" does not exist.
		  TO FIX: Use the "lpstat -p all" command to list
				  all known printers.
UX:lpstat: ERROR: Printer "dummy" does not exist.
		  TO FIX: Use the "lpstat -p all" command to list
				  all known printers.

$ /usr/lib/lp/local/lpstat -p
printer testprn disabled since Fri Jul 19 14:05:15 EDT 2002. available.
		new printer

$ /usr/lib/lp/local/lpstat -p -l
printer testprn disabled since Fri Jul 19 14:05:15 EDT 2002. available.
		new printer
		Form mounted:
		Content types: simple
		Printer types: unknown
		Description:
		Connection: direct
		Interface: /usr/lib/lp/model/standard
		On fault: write to root once
		After fault: continue
		Users allowed:
				(all)
		Forms allowed:
				(none)
		Banner required
		Character sets:
				(none)
		Default pitch:
		Default page size:
		Default port settings:

$ /usr/lib/lp/local/lpstat -p -D
printer testprn disabled since Fri Jul 19 14:05:15 EDT 2002. available.
		new printer
		Description:

$ /usr/lib/lp/local/lpstat -p -l -D
printer testprn disabled since Fri Jul 19 14:05:15 EDT 2002. available.
		new printer
		Form mounted:
		Content types: simple
		Printer types: unknown
		Description:
		Connection: direct
		Interface: /usr/lib/lp/model/standard
		On fault: write to root once
		After fault: continue
		Users allowed:
				(all)
		Forms allowed:
				(none)
		Banner required
		Character sets:
				(none)
		Default pitch:
		Default page size:
		Default port settings:

*/
static void lpstat_p(const char list[], gu_boolean option_l, gu_boolean option_D)
	{
	/*
	** Chug through the uprint-remote.conf file and print dummy lines for the
	** queues.  We don't have real acceptance times, so we print a fake time.
	*/
	{
	FILE *f;
	if((f = fopen(UPRINTREMOTECONF, "r")) != (FILE*)NULL)
		{
		char *line = NULL;
		int line_available = 80;
		while((line = gu_getline(line, &line_available, f)))
			{
			if(line[0] == '[')
				{
				line[strcspn(line, "]")] = '\0';
				if(!strchr(line+1, '?') && !strchr(line+1, '*'))
					printf("printer %s unknown state. enabled since Jan 1 00:00 1970. available.\n", line + 1);
				}
			}
		fclose(f);
		}
	}

	/*
	** Chug through the PPR printers, printing dummy acceptance messages as
	** (again with fake times) we go.
	*/
	{
	DIR *dir;
	struct dirent *direntp;
	int len;
	if((dir = opendir(PRCONF)))
		{
		while((direntp = readdir(dir)))
			{
			/* Skip . and .. and hidden files. */
			if(direntp->d_name[0] == '.')
				continue;

			/* Skip Emacs style backup files. */
			len = strlen(direntp->d_name);
			if( len > 0 && direntp->d_name[len-1]=='~' )
				continue;

			printf("printer %s unknown state. enabled since Jan 1 00:00 1970. available.\n", direntp->d_name);
			}
		}
	}
	}

/*

This function implements "lpstat -R" which lists all jobs in the queue
with a number next to each one.

$ /usr/lib/lp/local/lpstat -R
  1 testprn-109			  root				 270 Jul 19 14:24
  2 testprn-110			  root				 270 Jul 19 14:25

*/
static void lpstat_R()
	{
	}

/*
** This is a list of the options in the format required by getopt().
** We use the getopt() from the C library rather than PPR's because
** we want to preserve the local system semantics.  Is this a wise
** decision?  I don't know.
**
** Notice that lpstat breaks the POSIX parsing rule that an option
** can either require an argument or not, but can't have an optional
** argument.
**
** The options -a, -c, -f, -o, -p, -S, -u, and -v all take optional
** arguments.  We have to use special code to handle this.
*/
static const char *option_list = "acdDfloprRsStuv";

/*
** Here is a function to get the optional argument value.
*/
extern char *optarg;
extern int optind, opterr, optopt;
static char *get_arg(int argc, char *argv[])
	{
	if(optind < argc && argv[optind][0] != '-')
		{
		return argv[optind++];
		}
	else
		{
		return NULL;
		}
	}

/*
** And here we tie it all together.
*/
int main(int argc, char *argv[])
	{
	int c;
	gu_boolean option_l = FALSE;
	gu_boolean option_D = FALSE;

	/* Initialize internation messages library. */
	#ifdef INTERNATIONAL
	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
	#endif

	/* Trap loops: */
	if(uprint_loop_check(myname) == -1)
		return 1;

	/*
	** Parse the switches.  Note that we don't actually _do_ anything yet.
	** We want to make sure we understand the whole line.  If we don't
	** bail out or (if we have real-lpstat), exec() real-lpstat.
	*/
	while((c = getopt(argc, argv, option_list)) != -1)
		{
		switch(c)
			{
			case 'a':			/* acceptance status, optional argument */
				get_arg(argc,argv);
				break;

			case 'c':			/* list classes and members, optional argument */
				get_arg(argc,argv);
				break;

			case 'd':			/* report system default destination */
				break;

			case 'D':			/* include descriptions (-p) */
				option_D = TRUE;
				break;

			case 'f':			/* verify forms, optional argument */
				get_arg(argc,argv);
				break;

			case 'l':			/* long format, no arguments */
				option_l = TRUE;
				break;

			case 'o':			/* output request status, optional argument */
				get_arg(argc,argv);
				break;

			case 'p':			/* printer status, optional argument */
				get_arg(argc,argv);
				break;

			case 'r':			/* scheduler status */
				break;

			case 'R':			/* output request status, no argument */
				break;

			case 's':			/* summary status (lots of info) */
				break;

			case 'S':			/* character set verification, optional argument */
				get_arg(argc,argv);
				break;

			case 't':			/* -s plus more info */
				break;

			case 'u':			/* list requests for users, optional argument */
				get_arg(argc,argv);
				break;

			case 'v':			/* list printers and devices, optional argument */
				get_arg(argc,argv);
				break;

			default:			/* switch not supported by uprint-lpstat */
				fprintf(stderr, _("%s: unrecognized option: -%c\n"), myname, c);
				return 1;
			}
		}

	/*
	** This is the loop in which we actually do things.
	*/
	optind = 1;
	while((c = getopt(argc, argv, option_list)) != -1)
		{
		switch(c)
			{
			case 'a':			/* acceptance status, optional argument */
				lpstat_a(get_arg(argc,argv));
				break;

			case 'c':			/* list classes and members, optional argument */
				lpstat_c(get_arg(argc,argv));
				break;

			case 'd':			/* report system default destination */
				printf("system default destination: %s\n", uprint_default_destinations_lp());
				break;

			case 'D':			/* include descriptions (-p) */
				break;

			case 'f':			/* verify forms, optional argument */
				get_arg(argc,argv);
				break;

			case 'l':			/* long format, no arguments */
				break;

			case 'o':			/* output request status, optional argument */
				lpstat_o(get_arg(argc,argv));
				break;

			case 'p':			/* printer status, optional argument */
				lpstat_p(get_arg(argc,argv), option_l, option_D);
				break;

			case 'r':			/* scheduler status */
				fputs("scheduler is running\n", stdout);
				break;

			case 'R':			/* ??? */
				lpstat_R();
				break;

			case 's':			/* summary status (lots of info) */
				break;

			case 'S':			/* character set verification, optional argument */
				get_arg(argc,argv);
				break;

			case 't':			/* -s plus more info */
				break;

			case 'u':			/* list requests for users, optional argument */
				get_arg(argc,argv);
				break;

			case 'v':			/* list printers and devices, optional argument */
				get_arg(argc,argv);
				break;
			}
		}

	return 0;
	} /* end of main() */

/* end of file */

