/*
** ~ppr/src/include/filter_lp.h
** Copyright 1995, 1996 Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** This file was last modified 14 December 1996.
*/

/*
** Line printer emulator filter for PPR.
**
** There are many parameters in this file which may usefully be changed,
** however it is most often better to use the filter options to do it.
*/

/* Factors to convert various measurment systems to PostScript units. */
#define INCH 72.0
#define CM (72.0/2.54)

#define DEFAULT_MAX_WIDTH 300	/* maximum line length (wrap anything longer) */
#define DEFAULT_TAB_WIDTH 8	/* spaces per tab */

/* These sets the limits of pages that `look good'. */
#define DEFAULT_MIN_COLUMNS 70          /* format page for no fewer than this many columns */
#define DEFAULT_MIN_LINES 50            /* and no fewer than this many printable lines */
#define DEFAULT_MAX_LINES 100           /* any more than this is unreasonable */

/* Default minimum margins for portrait mode. */
#define DEFAULT_PMTM (0.5*INCH)	/* default top margin */
#define DEFAULT_PMBM (0.5*INCH)	/* default bottom margin */
#define DEFAULT_PMLM (0.5*INCH)	/* default left margin */
#define DEFAULT_PMRM (0.5*INCH)	/* default right margin */
#define DEFAULT_PDEFLINES 66	/* default lines per page */

/* Default minimum margins for landscape mode. */
#define DEFAULT_LMTM (0.375*INCH)	/* default minimum landscape top margin */
#define DEFAULT_LMBM (0.375*INCH)	/* default minimum landscape bottom margin */
#define DEFAULT_LMLM (0.375*INCH)	/* default minimum landscape left margin */
#define DEFAULT_LMRM (0.375*INCH)	/* default minimum landscape right margin */
#define DEFAULT_LDEFLINES 66		/* default lines per page */

#define DEFAULT_GUTTER (0.375*INCH)	/* default gutter width */

/* 
** These should only be changed if you can find another
** fixed space font.
*/
#define DEFAULT_CHAR_WIDTH 0.60		/* CHAR_WIDTH x pointsize = width */
#define DEFAULT_CHAR_HEIGHT 1.00	/* CHAR_HEIGHT x pointsize = height */
#define FONTNAME "Courier"		/* font to print in */
#define BFONTNAME "Courier-Bold"	/* font for overstruck text */
#define FONTNAME_CP437 "IBMCourier"
#define BFONTNAME_CP437 "IBMCourier-Bold"

/* Do not change parameters below this line. */
#define ATTR_BOLD 1             /* bit 0 is for bold */
#define ATTR_UNDERLINE 2        /* bit 1 is for underline */

/* Define the possible encodings. */
#define ENCODING_STANDARD 0
#define ENCODING_ISOLATIN1 1
#define ENCODING_CP437 2

/* end of file */
