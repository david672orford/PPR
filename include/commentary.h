/*
** mouse:~ppr/src/templates/module.c
** Copyright 1995--2001, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appears in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is"
** without express or implied warranty.
**
** Last modified 4 May 2001.
*/

/*
** The values for the "flags" argument to pprdrv's commentary()
** function.  These are used to indicate into which
** catagory the message falls so that commentary() can decide
** which of the commentators to pass the information to.
**
** Every commentary message will be in one and only one of these
** four categories.
*/
#define COM_PRINTER_ERROR 1		/* printer errors ( %%[ PrinterError: xxxxxx ]%% ) */
#define COM_PRINTER_STATUS 2		/* printer status messages ( %%[ status: xxxxxx ]%% ) */
#define COM_STALL 4			/* "printing bogged down" */
#define COM_EXIT 8			/* Why did it exit? */

/* end of file */
