/*
** mouse:~ppr/src/include/uprint_private.h
** Copyright 1995--1999, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Last modified 14 February 2000.
*/

/* Do not turn this on when linking with lprsrv!  Such
   operation is not supported yet. */
#if 0
#define DEBUG 1
#endif

#ifdef DEBUG
#define DODEBUG(a) uprint_debug a
void uprint_debug(const char *string, ...);
#else
#define DODEBUG(a)
#endif

/* Time to wait for connect() to finish: */
#define LPR_CONNECT_TIMEOUT 20

/* The file we use to get the next lpr jobid: */
#define LPR_PREVID_FILE RUNDIR"/lastid_uprint_lpr"

/* The file we use for a log: */
#define UPRINT_LOGFILE LOGDIR"/uprint"

/* The correct value for the signiture field in struct UPRINT: */
#define UPRINT_SIGNITURE 0x8391

/* uprint_sys5_to_bsd.c: */
int uprint_parse_lp_interface_options(void *p);
int uprint_parse_lp_filter_modes(void *p);

/* Some internal str_*[] length limits. */
#define MAX_MAILADDR 127
#define MAX_PRINCIPAL 127
#define MAX_FOR 127

/* A structure to describe a print job: */
struct UPRINT
	{
	int signiture;

	const char *fakername;		/* which fake program was used? */
	const char **argv;		/* argv[] from origional command */
	int argc;

	const char *dest;		/* the printer */
	const char **files;		/* list of files to print */

	uid_t uid;			/* user id number */
	const char *user;		/* user name */
	const char *from_format;	/* queue display from field format */
	const char *lpr_mailto;		/* lpr style address to send email to */
	const char *lpr_mailto_host;	/* @ host */
	const char *fromhost;		/* lpr style host name */
	const char *proxy_class;	/* string for after "@" sign in -X argument */
	const char *lpr_class;		/* lpr -C switch */
	const char *jobname;		/* lpr -J switch, lp -t switch */
	const char *pr_title;		/* title for pr (lpr -T switch) */

	const char *content_type_lp;	/* argument for lp -T or "raw" for -r */
	char content_type_lpr;		/* lpr switch such as -f, -c, or -d */
	int copies;			/* number of copies */
	gu_boolean banner;			/* should we ask for a banner page? */
	gu_boolean nobanner;			/* should we ask for suppression? */
	gu_boolean filebreak;
	int priority;			/* queue priority */
	gu_boolean immediate_copy;		/* should we copy file before exiting? */

	const char *form;		/* form name */
	const char *charset;		/* job character set */
	const char *width;
	const char *length;
	const char *indent;
	const char *cpi;
	const char *lpi;
	const char *troff_1;
	const char *troff_2;
	const char *troff_3;
	const char *troff_4;

	/* System V lp style options: */
	char *lp_interface_options;	/* lp -o switch */
	char *lp_filter_modes;		/* lp -y switch */
	const char *lp_pagelist;	/* lp -P switch */
	const char *lp_handling;	/* lp -H switch */

	/* DEC OSF/1 style options: */
	const char *osf_LT_inputtray;
	const char *osf_GT_outputtray;
	const char *osf_O_orientation;	/* portrait or landscape */
	const char *osf_K_duplex;
	int nup;	 		/* N-Up setting */

	gu_boolean unlink;			/* should job files be unlinked? */
	gu_boolean show_jobid;		/* should be announce the jobid? */
	gu_boolean notify_email;		/* send mail when job complete? */
	gu_boolean notify_write;		/* use write(1) when job complete? */

	/* PPR options: */
	const char *ppr_responder;
	const char *ppr_responder_address;
	const char *ppr_responder_options;

	/* Scratch space: */
	char str_numcopies[5];
	char str_typeswitch[3];
	char str_mailaddr[MAX_MAILADDR + 1];
	char str_principal[MAX_PRINCIPAL + 1];		/* argument for ppr -X switch */
	char str_for[MAX_FOR + 1];				/* argument for ppr -f switch */
	char str_pr_title[6 + LPR_MAX_T + 1];		/* space for "title=my title" */
	char str_width[12];
	char str_length[12];
	char str_lpi[12];
	char str_cpi[12];
	char str_priority[3];
	char str_inputtray[sizeof("*InputSlot ") + LPR_MAX_DEC + 1];
	char str_outputtray[sizeof("*OutputBin ") + LPR_MAX_DEC + 1];
	char str_orientation[sizeof("orientation=") + LPR_MAX_DEC + 1];
	char str_nup[4];
	} ;

/* Description of a table which translates lpr to lp types: */
struct LP_LPR_TYPE_XLATE
	{
	const char *lpname;
	char lprcode;
	} ;

extern struct LP_LPR_TYPE_XLATE lp_lpr_type_xlate[];

/* end of file */

