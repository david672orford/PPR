/*
** mouse:~ppr/src/pprdrv/pprdrv_tt.c
** Copyright 1995--2000, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Last revised 21 September 2000.
*/

/*
** This modules has is used for TrueType font support.	It has one function
** which can be called to make a best effort to make sure a TrueType 
** rasterizer is present.  The other function converts a Microsoft Windows
** .TTF file to either a Type 3 or a Type 42 PostScript font.
** 
*/

#include "before_system.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include "gu.h"
#include "global_defines.h"

#include "pprdrv.h"
#include "interface.h"
#include "libttf.h"

/*
** If this routine is called, we should make our best effort to provide
** a TrueType rasterizer.  If the PPD file contains a "*TTRasterizer: Type42"
** line then we need do nothing special.  If the PPD file contains a
** "*TTRasterizer: Accept68K" line then TrueType is ok only if the job
** contains TrueDict or we can arrange to have it downloaded.  If the PPD
** file contains any other "*TTRasterizer:" line or none at all then we must
** assume that TrueType fonts are not acceptable.
*/
void want_ttrasterizer(void)
	{
	static int decided = FALSE;

	if( ! decided )
		{
		decided = TRUE;

		if(Features.TTRasterizer == TT_TYPE42)
			{
			printer.type42_ok = TRUE;
			}

		else if(Features.TTRasterizer == TT_ACCEPT68K)
			{
			int x;

			/*
			** If this is a Laserwriter 8.x job which already contains
			** TrueDict, that is acceptable.
			*/
			for(x=0; x < drvres_count; x++)
				{
				if(strcmp(drvres[x].type,"file")==0 && strcmp(drvres[x].name,"adobe_psp_TrueType")==0 )
					{
					printer.type42_ok = TRUE;
					return;
					}
				}

			/*
			** If we can arrange to have TrueDict downloaded,
			** then type 42 fonts are acceptable.
			*/
			if(add_resource("procset", "TrueDict", 27, 0) == 0)
				printer.type42_ok = TRUE;
			else
				printer.type42_ok = FALSE;
			}

		else									/* TTRaserizer "None" or Unknown: */
			{									/* don't allow type 42 fonts. */
			printer.type42_ok = FALSE;
			}
		}
	} /* end of want_ttrasterizer() */

/*
** Use libttf to insert the TrueType font into the output stream.
*/
void send_font_tt(const char filename[])
	{
	const char function[] = "send_font_tt";
	void *font;
	int target_type;

	DODEBUG_TRUETYPE(("%s(\"%s\")", function, filename));

	{
	TTF_RESULT ttf_result;
	if((ttf_result = ttf_new(&font, filename)) != TTF_OK)
		fatal(EXIT_TTFONT, "%s(): ttf_new() failed for %s, %s", function, filename, ttf_strerror(ttf_result));
	}

	/* Decide what type of PostScript font we will be generating. */
	if(printer.type42_ok)
		target_type = 42;
	else
		target_type = 3;

	/*
	** These words are inserted before the "%!" signiture so
	** that if this output file is every fed to PPR as input,
	** PPR will not cache the font.	 This is important because
	** if the font were cached, PPR would lose its ability to
	** generate the font in a format acceptable to a particular
	** printer.
	*/
	printer_puts("%PPR-don't-cache ");	/* <-- note absence of newline! */

	/*
	** This call converts the font to PostScript and sends
	** it to the output routines defined above.
	*/
	if(ttf_psout(font, printer_putc, printer_puts, printer_printf, target_type) == -1)
		fatal(EXIT_TTFONT, "%s(): ttf_psout() failed for %s, %s", function, filename, ttf_strerror(ttf_errno(font)));

	/*
	** Close the file and free the memory.
	*/
	if(ttf_delete(font) == -1)
		fatal(EXIT_TTFONT, "%s(): ttf_delete() failed, %s", function, ttf_strerror(ttf_errno(font)));
	} /* end of send_font_tt() */

/* end of file */

