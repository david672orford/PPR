/*
** mouse:~ppr/src/include/ppr_exits.h
** Copyright 1995--2000 Trinity College Computing Center
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is"
** without express or implied warranty.
**
** Last modified 19 June 2000.
*/

/*
** Exit codes for the ppr utility.  These are of interest not only
** to the parts of ppr which must exit with these codes but also
** to program such as papsrv and lprsrv which invoke ppr.
*/

#define PPREXIT_OK 0			/* normal exit */
#define PPREXIT_NOCHARGEACCT 1		/* failed to find charge account */
#define PPREXIT_BADAUTH 2		/* wrong authcode */
#define PPREXIT_OVERDRAWN 3		/* account is overdrawn */
#define PPREXIT_NONCONFORMING 4		/* bad DSC, can't count pages */
#define PPREXIT_DISKFULL 5		/* Disk got full */
#define PPREXIT_BADHEADER 6		/* unterminated dot or PJL header */
#define PPREXIT_TRUNCATED 7		/* Input file had no %%EOF */
#define PPREXIT_NOMATCH 8		/* media couldn't be matched */
#define PPREXIT_ACL 9			/* ACL forbids */

#define PPREXIT_NOFILTER 10		/* proper filter not available */
#define PPREXIT_NOSPOOLER 11		/* spooler not running */
#define PPREXIT_OTHERERR 12		/* other internal error */
#define PPREXIT_CONFIG 13		/* ppr.conf problem */

#define PPREXIT_SYNTAX 20		/* invokation syntax error */

#define PPREXIT_KILLED 30		/* received a fatal signal */

/* end of file */
