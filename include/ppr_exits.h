/*
** mouse:~ppr/src/include/ppr_exits.h
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
** Last modified 14 December 2004.
*/

/*
** Exit codes for the ppr utility.  These are of interest not only
** to the parts of PPR which must exit with these codes but also
** to program such as papd and lprsrv which invoke ppr.
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
#define PPREXIT_NOTPOSSIBLE 22		/* request not executable */

#define PPREXIT_KILLED 30		/* received a fatal signal */

/* end of file */
