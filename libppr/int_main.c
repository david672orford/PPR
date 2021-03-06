/*
** mouse:~ppr/src/libppr/int_main.c
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
** Last modified 13 January 2005.
*/

#include "config.h"
#include <stdarg.h> 
#include <string.h>
#include <stdlib.h>
#include "gu.h"
#include "global_defines.h"
#include "libppr_int.h"
#include "interface.h"

/** exception catching wrapper main() function
 *
 * This wraps the interface's main() function in order to install a special 
 * exception handler which writes the exception to the printer's alerts log.
 *
 * Any program that provides its own main() won't link this in.  To use this
 * function, you should omit main() and provided an int_main() instead.
 */
int main(int argc, char *argv[])
	{
	gu_Try {
		return int_main(argc, argv);
		}
	gu_Catch {
		alert(int_cmdline.printer, FALSE, "%s", gu_exception);

		if(strstr(gu_exception, "alloc()") || strstr(gu_exception, "fork()"))
			exit(EXIT_STARVED);
		else
			exit(EXIT_PRNERR_NORETRY);
		}
	/* NOTREACHED */
	return 255;
	} /* end of main() */

/* end of file */

