/*
** mouse:~ppr/src/pprdrv/pprdrv_pfb.c
** Copyright 1995--2004, Trinity College Computing Center.
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
** Last modified 28 May 2004.
*/

#include "before_system.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"

#include "interface.h"
#include "pprdrv.h"

/*
** This routine, which is called by internal_include_resource() will uncompress
** the PFB font data in the file and write it to the printer.
**
** We need the file name for error messages.
*/
void send_font_pfb(const char filename[], FILE *ifile)
	{
	int c;						/* temporary character storage */
	int larray[4];				/* don't us char! */
	unsigned int length;		/* the computed length */
	int x;						/* loop counter */
	static const char hexchars[] = "0123456789ABCDEF";
	int segment_type;
	int linelength;				/* for breaking hexadecimal lines */

	/* We go around this loop once for each segment. */
	while((c = fgetc(ifile)) != EOF)			/* should never be false */
		{
		if(c != 128)
			fatal(EXIT_PRNERR_NORETRY, _("PFB file \"%s\" is corrupt (defect type %d)"), filename, 1);

		segment_type = fgetc(ifile);			/* should be 1, 2, or 3 */

		length = 0;
		if(segment_type == 1 || segment_type == 2)
			{
			for(x=0; x < 4; x++)				/* read 4 byte length */
				{
				if((larray[x] = fgetc(ifile)) == EOF)
					fatal(EXIT_PRNERR_NORETRY, _("PFB file \"%s\" is corrupt (defect type %d)"), filename, 2);
				}
			for(x=3; x >= 0; x--)				/* interpret as little-endian */
				{
				length *= 256;
				length += larray[x];
				}
			}

		switch(segment_type)
			{
			case 1:						/* ASCII segment */
				while(length--)
					{
					if((c = fgetc(ifile)) == EOF)
						fatal(EXIT_PRNERR_NORETRY, _("PFB file \"%s\" is corrupt (defect type %d)"), filename, 3);

					if(c == '\r') c = '\n';		/* some fonts use \r, others \n */

					printer_putc(c);
					}
				break;
			case 2:						/* Hexadecimal data segment */
				linelength=0;
				while(length--)
					{
					if((c = fgetc(ifile)) == EOF)
						fatal(EXIT_PRNERR_NORETRY, _("PFB file \"%s\" is corrupt (defect type %d)"), filename, 4);

					printer_putc( hexchars[c/16] );
					printer_putc( hexchars[c%16] );

					if(++linelength >= 40)
						{
						printer_putc('\n');
						linelength=0;
						}
					}
				break;
			case 3:						/* end of file */
				return;
			default:					/* unknown segment type */
				fatal(EXIT_PRNERR_NORETRY, _("PFB file \"%s\" is corrupt (defect type %d)"), filename, 5);
			}

		}

	fatal(EXIT_PRNERR_NORETRY, _("PFB file \"%s\" is corrupt (defect type %d)"), filename, 6);
	} /* end of send_font_pfb() */

/* end of file */

