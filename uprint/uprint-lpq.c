/*
** mouse:~ppr/src/uprint/uprint-lpq.c
** Copyright 1995--2003, Trinity College Computing Center.
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
** Last modified 18 February 2003.
*/

#include "before_system.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <pwd.h>
#ifdef INTERNATIONAL
#include <locale.h>
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "uprint.h"

extern char *optarg;
extern int optind;

/* This name will appear in certain error messages: */
static const char *const myname = "uprint-lpq";

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
** In main() we parse the options and call the
** uprint library routine to list a queue.
*/
static const char *option_list = "P:l";

int main(int argc, char *argv[])
	{
	int c;
	const char *queue = (const char *)NULL;
	int format = 0;

	/* Initialize internation messages library. */
	#ifdef INTERNATIONAL
	setlocale(LC_MESSAGES, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
	#endif

	/* Trap loops: */
	if(uprint_loop_check(myname) == -1)
		return 1;

	/*
	** Parse the switches.	Mostly, we will call uprint
	** member functions.
	*/
	while((c = getopt(argc, argv, option_list)) != -1)
		{
		switch(c)
			{
			case 'P':			/* printer */
				queue = optarg;
				break;

			case 'l':			/* long format */
				format = 1;
				break;

			default:
				/* fprintf(stderr, _("%s: Syntax error, unrecognized switch: -%c\n"), myname, c); */
				return 1;
			}
		}

	/* If the print destination has not yet been
	   determined, determine it now. */
	if(queue == (const char *)NULL)
			queue = uprint_default_destinations_lpr();

	{
	int return_code;

	/* Print the queue. */
	if((return_code = uprint_lpq((uid_t)-1, (gid_t)-1, "???", queue, format, (const char **)&argv[optind], TRUE)) != -1)
		{
		/* Child ran, nothing more to do. */
		}
	/* Unclaimed: */
	else if(uprint_errno == UPE_UNDEST)
		{
		fprintf(stderr, _("%s: Print queue \"%s\" not found.\n"), myname, queue);
		return_code = 1;		/* <-- exit code that BSD lpq uses for unknown queue */
		}
	/* Command failed: */
	else
		{
		/* An arbitrary value intended to indicated a major failure: */
		return_code = 255;
		}

	return return_code;
	}
	} /* end of main() */

/* end of file */

