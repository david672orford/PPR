/*
** mouse:~ppr/src/include/respond.h
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
** Last modified 6 December 2000.
*/

/*
** Types of responses to send to the user:
*/
#define RESP_FINISHED 0				/* job printed normally */
#define RESP_ARRESTED 1				/* job placed on hold due to error */
#define RESP_CANCELED 2				/* canceled while not printing */
#define RESP_CANCELED_PRINTING 3		/* canceled while printing */
#define RESP_CANCELED_BADDEST 4			/* canceled because bad destination */
#define RESP_CANCELED_REJECTING 5		/* destination is in reject mode */
#define RESP_CANCELED_NOCHARGEACCT 6		/* user no charge account */
#define RESP_CANCELED_BADAUTH 7			/* incorrect authcode */
#define RESP_CANCELED_OVERDRAWN 8		/* overdraft too large */
#define RESP_STRANDED_PRINTER_INCAPABLE 9	/* exceeds printer capabilities */
#define RESP_STRANDED_GROUP_INCAPABLE 10	/* exceeds capabilities of each member */
#define RESP_CANCELED_NONCONFORMING 11		/* DSC not good enough */
#define RESP_NOFILTER 12			/* suitable filter not available */
#define RESP_FATAL 13				/* Unspecified fatal ppr error */
#define RESP_NOSPOOLER 14			/* PPRD not running */
#define RESP_BADMEDIA 16			/* unrecognized media */
#define RESP_BADPJLLANG 17			/* PJL header specifies unknown language */
#define RESP_FATAL_SYNTAX 18			/* ppr command invoked with bad syntax */
#define RESP_CANCELED_NOPAGES 19		/* can't honour page list */
#define RESP_CANCELED_ACL 20			/* ACL forbids queue access */

/* end of file */
