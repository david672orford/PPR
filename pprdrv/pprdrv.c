/*
** mouse:~ppr/src/pprdrv/pprdrv.c
** Copyright 1995--2002, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Last modified 18 April 2002.
*/

/*
** This program is called by pprd to do the actual printing.  It launches the
** interface program and resembles the job and sends it to the interface
** program.
*/

#include "before_system.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>		/* for gettimeofday() */
#ifdef INTERNATIONAL
#include <locale.h>
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "global_structs.h"
#include "pprdrv.h"
#include "interface.h"
#include "userdb.h"
#include "version.h"

/*====================================================
** Global variables.
====================================================*/

/* Have we caught SIGTERM?  If it is every set to true, then we shut down. */
volatile gu_boolean sigterm_caught = FALSE;

/* Has one of our timeouts expired?  No need to initialize this. */
volatile gu_boolean sigalrm_caught;

/* Are we running with the --test switch? */
int test_mode = FALSE;

/* Queue entry variables. */
const char *QueueFile;				/* name of queue file of job to print */
FILE *qstream;					/* queue file "handle" */
struct QFileEntry job;				/* queue file entry */

/* List of resource refered to in this job. */
struct DRVRES *drvres = (struct DRVRES *)NULL;
int drvres_count = 0;
int drvres_space = 0;

/* List of DSC requirements. */
char *drvreq[MAX_DRVREQ];
int drvreq_count = 0;

/* TRUE if sending main part of job, not flag pages or queries. */
gu_boolean doing_primary_job = FALSE;

/* For begin_feature() */
gu_boolean doing_prolog = FALSE;
gu_boolean doing_docsetup = FALSE;

/* Line buffers */
char line[MAX_LINE+1];		/* input buffer (MAX_LINE plus room for NULL) */
int line_len;			/* length of current line in bytes */
int line_overflow;		/* true or false */
static char pline[256];		/* buffer for "-pages" file lines */

int level = 0;			/* Document nesting level */

FILE *comments;			/* header comments file */
FILE *page_comments;		/* page level comments file */
FILE *text;			/* other parts of file */

struct PPRDRV printer;		/* defines things about the printer */

/*
** Things we want to strip out because we are doing it some other way:
** These are used to control begin_feature().  They are only turned
** one during the part of the job we want the behaviour in.
*/
int strip_binselects = FALSE;	/* TRUE when begin_feature() should strip bin selection code */
int strip_signature = FALSE;	/* TRUE when begin_feature() should strip signature features */

/*
** Variables needed to re-order pages.
**
** current_job_direction is the same as job.attr.pageorder except
** that when the latter has the value 0, the former has the value 1.
**
** print_direction is 1 if we will send the pages in ascending order,
** -1 if we will send them in descending order.  If the PageOrder
** is "Special", print_direction will be the same as current_job_direction.
*/
int current_job_direction;	/* 1 if input is forward, -1 if input is backwards */
int print_direction;		/* final page emmission order, 1 if ascend, -1 if descend */

/*
** The number of sheets in each copy of the job.
** This variable is set in page_computations().
** It is referred to when re-ordering pages and
** at other times.
*/
int sheetcount;

/*===================================================
** Static variables (Those having a scope limited
** to this source module.
===================================================*/

/* The time at which we started running.  This
   is used in some debug messages. */
struct timeval start_time;

/*
** These three variables define how we will print copies.
** The first is actually global because pprdrv_req.c must
** read it.
*/
int copies_auto;			/* TRUE if should define #copies */
static int copies_doc_countdown;	/* number of times to send document */
static int copies_pages_countdown;	/* number of times to send pages */

/* The "%%PageMedia:" from the document defaults section. */
static char default_pagemedia[MAX_MEDIANAME+1] = {(char)NULL};

/* Array used to change page order. */
struct PAGETABLE
	{
	long int pages_offset;		/* offset into the "-pages" file */
	long int text_offset;		/* offset into the "-text" file */
	};
static struct PAGETABLE *pagetable;

/*=============================================================
** Routines for re-assembling the job.
=============================================================*/

/*
** Copy data between "%%BeginData:" and "%%EndData" comments.
*/
static void copy_data(FILE *infile)
    {
    long int len;
    int c;

    /* copy "%%BeginData:" thru to output */
    printer_putline(line);

    /* If the "%%BeginData:" line didn't contain enough
       to go on, don't do anything more. */
    if(!tokens[1] || !tokens[2] || !tokens[3])
    	return;

    sscanf(tokens[1], "%ld", &len);		/* read the number of items */

    if(strcmp(tokens[3], "Lines") == 0)		/* find if lines or bytes */
	{					/* lines */
	while(len--)
	    {
	    if(!fgets(line, sizeof(line), infile))
		break;
	    printer_puts(line);		    /* this doesn't change line termination */
	    }
	}
    else                                    /* bytes */
	{
	while(len-- && (c = fgetc(infile)) != EOF)
	    {
	    printer_putc(c);
	    }
	}
    } /* end of copy_data() */

/*-------------------------------------------------------------------------
** Sanitized getline: dgetline()
** This routine reads from a specified file.
** It is intended that the file be either "-text" or "-comments".
** This routine removes the line termination.
**
** Also, in the case of the comments "%%IncludeFeature:" and "%%BeginFeature:",
** "%%BeginResource:", "%%IncludeResource:", and "%%BeginData:",
** this routine passes them thru to the destination file immediatly, the
** higher level routines never see them.  It also increments the variable
** level when it sees "%%BeginDocument:" and decrements it at "%%EndDocument".
-------------------------------------------------------------------------*/

/*
** Read a line from the specified file into the line buffer.
** Return its length in line_len.
**
** The length of line[] is defined above as MAX_LINE+1.
*/
static char *dgetline_read(FILE *infile)
    {
    int c;				/* the character read */

    line_overflow = FALSE;		/* we don't yet know that it will overflow */

    for(line_len=0; line_len<MAX_LINE; line_len++)
    	{
	if((c = fgetc(infile)) == EOF)	/* if end of file */
	    {
	    if(line_len)		/* If we have read a line */
	    	break;			/* already, EOF is termination. */
	    else			/* otherwise, */
	        return (char*)NULL;	/* return a null pointer as end of file. */
	    }

	if(c == '\n')			/* If end of line, */
	    break;			/* break off. */

	line[line_len] = c;		/* a character, store it */
    	}

    line[line_len] = '\0';		/* terminate the line */

    if(line_len == MAX_LINE)		/* If for loop terminated because the */
    	line_overflow = TRUE;		/* line was too long, then say so. */

    return line;			/* return pointer to the line */
    } /* end of dgetline_read() */

/*
** If the line should not be returned to the caller, this
** routine returns -1, otherwise it returns 0.
**
** This routine quitely copies feature code, documents, and resources.
** End feature or resource marker lines which occur without begin
** lines are unlikely to have any effect.
*/
static int dgetline_parse(FILE *infile)
    {
    if(*line == '%')			/* If it is a comment, */
	{				/* look for specific comments */
	if(lmatch(line, "%%BeginFeature:"))
	    {
	    tokenize();
	    begin_feature(tokens[1], tokens[2], infile);
	    return -1;
	    }
	if(lmatch(line, "%%IncludeFeature:"))
	    {
	    tokenize();
	    include_feature(tokens[1], tokens[2]);
	    return -1;
	    }
	if(lmatch(line, "%%IncludeResource:"))
	    {
	    tokenize();
	    include_resource();
	    return -1;
	    }
	if(lmatch(line, "%%BeginResource:"))
	    {
	    tokenize();
	    begin_resource(infile);     /* copy the resource */
	    return -1;                  /* including %%EndResource */
	    }
	if(lmatch(line, "%%BeginData:"))
	    {
	    tokenize();
	    copy_data(infile);
	    return -1;
	    }

	if(lmatch(line, "%%BeginDocument:"))
	    level++;
	else if( (strcmp(line, "%%EndDocument") == 0) && level)
	    level--;

	} /* end of if comment */
    return 0;
    } /* end of dgetline_parse() */

char *dgetline(FILE *infile)
    {
    char *lptr;

    do  {
	lptr = dgetline_read(infile);
	} while(lptr && *lptr && dgetline_parse(infile));
			/* this skips parsing for blank lines... interesting */
    return lptr;
    } /* end of dgetline() */

/*----------------------------------------------------------------------
** Routines to emmit the early part of the job, the part before the
** script section.
**--------------------------------------------------------------------*/

/*
** Copy the header comments from
** the -comments file to the output file.
*/
static void copy_header(void)
    {
    /*
    ** Copy the "-comments" file to the printer.  The first
    ** that will be copied is the "%!" line.
    */
    while(dgetline(comments))
	printer_putline(line);

    /*
    ** Possibly call a user-written module to add additional comments.
    */
    custom_hook(CUSTOM_HOOK_COMMENTS, 0);

    /*
    ** Reconstruct some comments which ppr removed.
    */
    printer_printf("%%%%For: (%s)\n", job.For ? job.For : "???");

    if(job.Creator)
	printer_printf("%%%%Creator: (%s)\n", job.Creator);

    if(job.Routing)
	printer_printf("%%%%Routing: %s\n", job.Routing);

    if(job.Title)
	printer_printf("%%%%Title: (%s)\n", job.Title);

    /*
    ** If the number of pages is not unspecified, then print it as a DSC
    ** comment.  If the number of copies is 1 or copies_pages_countdown starts
    ** at 1, then the complicated formula below is reduced to "pages".
    */
    if(job.attr.pages != -1)
	{
	int pages = pagemask_count(&job);
	printer_printf("%%%%Pages: %d\n",
       	    ((sheetcount * pages * (copies_pages_countdown-1))+pages)); /* !!! */
	}

    /*
    ** If we are not re-ordering the job or we are re-ordering
    ** the job but we are not doing duplex or N-Up (which make
    ** for a very confusing page order), print the pageorder.
    ** For now, we will overlook the strange aspects of a document
    ** produced by a copies_pages > 1.
    **
    ** The expression which is fed to the switch is rather odd.
    ** The first part, (job.attr.pageorder * current_job_direction)
    ** will always evaluate to 1 or 0.
    */
    if(print_direction == 1 || job.attr.pagefactor == 1)
	{
	switch(job.attr.pageorder * current_job_direction * print_direction)
	    {
	    case PAGEORDER_ASCEND:                  /* 1 */
		printer_puts("%%PageOrder: Ascend\n");
		break;
	    case PAGEORDER_DESCEND:                 /* -1 */
		printer_puts("%%PageOrder: Descend\n");
		break;
	    case PAGEORDER_SPECIAL:                 /* 0 */
		printer_puts("%%PageOrder: Special\n");
		break;
	    }
	}

    /* Orientation */
    switch(job.attr.orientation)
    	{
	case ORIENTATION_PORTRAIT:
	    printer_puts("%%Orientation: Portrait\n");
	    break;
	case ORIENTATION_LANDSCAPE:
	    printer_puts("%%Orienation: Landscape\n");
	    break;
    	}

    /* Write the ``%%DocumentNeeded(Supplied)Resources:'' comments. */
    write_resource_comments();

    /* Write the ``%%Requirements:'' comments. */
    write_requirement_comments();

    /* Write the ProofMode */
    switch(job.attr.proofmode)
    	{
    	case PROOFMODE_TRUSTME:
	    printer_puts("%%ProofMode: TrustMe\n");
	    break;
	case PROOFMODE_SUBSTITUTE:
	    printer_puts("%%ProofMode: Substitute\n");
	    break;
	case PROOFMODE_NOTIFYME:
	    printer_puts("%%ProofMode: NotifyMe\n");
	    break;
	}

    /* Character code usage: */
    switch(job.attr.docdata)
	{
	case CODES_Clean7Bit:
	    printer_puts("%%DocumentData: Clean7Bit\n");
	    break;
	case CODES_Clean8Bit:
	    printer_puts("%%DocumentData: Clean8Bit\n");
	    break;
	case CODES_Binary:
	    printer_puts("%%DocumentData: Binary\n");
	    break;
	default:
	    break;
	}

    /* Leave a sign that PPR was here: */
    printer_puts("%" PPR_DSC_PREFIX "Spooler: PPR-" SHORT_VERSION "\n");

    /* Finally, explicitly close the comments section: */
    printer_puts("%%EndComments\n");
    } /* end of copy_header() */

/*
** Copy the document defaults section from the start of the
** "-pages" file.  This will be the first instance of reading
** from the "-pages" file.  If there are no defaults, we will
** rewind the file.
*/
static void copy_defaults(void)
    {
    int defaults=0;

    while(dgetline(page_comments))			/* read a line */
	{
	if(! defaults)					/* if not found yet */
	    {
	    if(strcmp(line, "%%BeginDefaults") == 0)	/* If it is present, */
		{					/* then ok. */
		printer_putc('\n');			/* put a line before "%%BeginDefaults" */
		defaults = TRUE;
		}
	    else					/* Otherwise, */
		{					/* rewind the */
		rewind(page_comments);			/* "-pages" file, */
		break;					/* and get out of here. */
		}
	    }

	printer_printf("%s\n", line);			/* sent the line to printer */

	gu_sscanf(line,"%%%%PageMedia: %#s",sizeof(default_pagemedia),default_pagemedia);

	if(strncmp(line,"%%EndDefaults",13)==0)		/* If it was the last line, */
	    break;					/* then stop. */
	}
    } /* end of copy_defaults() */

/*
** Copy the prolog to the output file up to and including the "%%EndProlog:"
** line.  If we hit EOF or "%%Trailer:", then we will return FALSE.
*/
static gu_boolean copy_prolog(void)
    {
    gu_boolean retval = FALSE;
    doing_prolog = TRUE;

    /*
    ** Copy blank lines before inserting extra lines.
    ** We must use a special version of dgetline() which
    ** does not automatically copy commented resources.
    ** This is important because otherwise, it might
    ** copy a procedure set before we get to add our's.
    */
    while(dgetline_read(text) && *line == '\0')
	printer_putline(line);

    /*
    ** If the 1st non-blank line was "%%BeginProlog",
    ** write it to the output file before inserting extra code.
    */
    if(strcmp(line, "%%BeginProlog") == 0)
	{                       /* If it is "%%BeginProlog", */
	printer_putline(line);         /* write to output */
	dgetline_read(text);         /* and read the next line. */
	}

    /*
    ** Give the code in pprdrv_patch.c a chance to load
    ** any *JobPatchFile sections from the PPD file.
    */
    jobpatchfile();

    /* Insert things in the prolog if we haven't already. */
    if(job.attr.prolog)
	{
	/* If we will need the N-Up library or some other
	   extra resource, insert it here. */
	insert_extra_prolog_resources();
	}

    /* The line in the buffer didn't get parsed above,
       do it now.  This may require us to get additional
       lines to satisfy things such as resource inclusion
       handling. */
    while(dgetline_parse(text))
	{
	if(! dgetline_read(text))
	    break;
	}

    /*
    ** Now, start the main copy loop.  Some of these cases are probably unnecessary
    ** since ppr (the program that places jobs in the queue) probably ensures that
    ** there is a "%%EndProlog" before the first "%%Page:".
    */
    do  {
	if(level==0 && strcmp(line, "%%BeginSetup") == 0)
	    {
	    retval = TRUE;
	    break;
	    }
	if(level==0 && lmatch(line, "%%Page:"))
	    {
	    retval = TRUE;		/* was FALSE, an error hidden by copy_trailer() */
	    break;
	    }
	if(level==0 && strcmp(line, "%%Trailer") == 0)
	    {
	    retval = FALSE;
	    break;
	    }
	if(level==0 && strncmp(line, "%%EndProlog", 11) == 0)
	    {
	    dgetline(text);	/* <-- Leave first line of next section in line[]. */
	    retval = TRUE;
	    break;
	    }

	/* copy the line through to the printer */
	printer_write(line, line_len);
	if(!line_overflow)
	    printer_putc('\n');

	} while(dgetline(text));

    if(retval)
	{
	custom_hook(CUSTOM_HOOK_PROLOG, 0);
	printer_puts("%%EndProlog\n");
	}

    doing_prolog = FALSE;
    return retval;
    } /* end of copy_prolog() */

/*
** Copy the Document Setup section.  Return FALSE if we hit the %%Trailer
** comment or end of file.  Return TRUE if we hit and copy "%%EndSetup".
*/
static gu_boolean copy_setup(void)
    {
    doing_docsetup = TRUE;

    /* Copy blank lines before inserting bin select code. */
    while(*line == '\0')				/* while line is zero length */
	{
	printer_putline(line);				/* copy it to the output */
	if(!dgetline_read(text))			/* and get the next one */
	    break;					/* defering parsing */
	}

    /* Copy "%%BeginSetup" line before inserting bin select code. */
    if(strcmp(line, "%%BeginSetup") == 0)
	{
	printer_putline(line);
	dgetline_read(text);		/* and get the next one, */
	}				/* defering parsing */

    /*
    ** Set the job name so that others who want to use the printer will be
    ** able to see who is using it and come looking for our blood.
    */
    set_jobname();

    /* Do this now as the document setup code may select fonts. */
    insert_noinclude_fonts();

    /*
    ** If this document appears to require only one medium and
    ** ppr was invoked with the "-B true" switch (or no -B switch),
    ** then insert bin select code at the top of the document
    ** setup section.  If we insert such code, we will strip
    ** out any pre-existing code.
    */
    if(media_count == 1 && job.opts.binselect)
	strip_binselects = select_medium(media_xlate[0].pprname);

    /*
    ** If we are printing signatures ourselves, then strip out any code
    ** in the document which might invoke such features.
    ** (job.N_Up.sigsheets is zero if we are not printing signatures.)
    */
    strip_signature = job.N_Up.sigsheets;

    /*
    ** Now insert those -F switch things which go at the beginning
    ** of the document setup section (set 1).
    ** Most should go here because those which select a page region
    ** do an implicit initmatrix which should occur before the
    ** document setup code scales the coordinate system.
    **
    ** This will set strip_binselects to TRUE if the inserted features
    ** include "*InputSlot" features.
    */
    insert_features(qstream, 1);

    /*
    ** If we are using N-Up, insert the N-Up invokation.  This has been
    ** placed after the auto bin select because the N-Up package makes
    ** itself at home on the currently selected media.  This is after
    ** insert_noinclude_fonts() so that the `Draft' notice font is
    ** downloaded before N-Up is invoked.
    */
    invoke_N_Up();

    /*
    ** We must now do the line parsing we defered above.  This may
    ** require that we read more lines.
    */
    while(dgetline_parse(text))
	{
	if(!dgetline_read(text))
	    break;
	}

    /*
    ** Now, copy the origional document setup section.
    **
    ** "%%EOF" should not occur since ppr should make sure all
    ** files have a "%%Trailer" comment.  We do not have to look
    ** for "%%Page:" comments because any file which has any
    ** pages will already have had "%%EndSetup" added.
    */
    do  {
	if(*line=='%' && level==0 && strcmp(line, "%%Trailer") == 0)
	    {
	    strip_binselects = FALSE;	/* stop stripping */
	    strip_signature = FALSE;
	    doing_docsetup = FALSE;
	    return FALSE;		/* then no pages. */
	    }				/* if we had hit %%EndSetup we would have returned 0.) */

	if(*line=='%' && level==0 && strncmp(line, "%%EndSetup", 10) == 0)
	    {
	    insert_features(qstream,2);	/* insert ppr -F *Duplex switch things */

	    if(job.opts.copies != -1)   /* specify number of copies */
		{
		if(copies_auto)         /* if auto, insert auto code */
		    set_copies(job.opts.copies);
		else                    /* if not auto copies, */
		    set_copies(1);      /* disable any old auto copies code */
		}

	    insert_userparams();

	    custom_hook(CUSTOM_HOOK_DOCSETUP, 0);

	    printer_puts("%%EndSetup\n"); /* print the "%%EndSetup" */

	    dgetline(text);             /* get 1st line for page routine */

	    strip_binselects = FALSE;	/* stop stripping binselects */
	    strip_signature = FALSE;
	    doing_docsetup = FALSE;
	    return TRUE;		/* Return 0 because we hit %%EndSetup, */
	    }     			/* there will be pages. */

	printer_write(line, line_len);
	if(!line_overflow)
	    printer_putc('\n');
	} while(dgetline(text));

    /*
    ** This assertion looks ok, but it is not a good idea.  Some highly
    ** defective PostScript files contain unclosed resources in the setup
    ** section.  This will cause the code above to speed right past
    ** the "%%Trailer" line (as it should).  Since ppr doesn't always fix
    ** them (even with editps turned on) and they will still print, though
    ** without the benefit of features that are dependent on usable "%%Page:"
    ** comments, we shouldn't throw an assertion failure here.
    */
    /* fatal(EXIT_JOBERR, "queue -text file lacks \"%%%%Trailer\" comment"); */

    return FALSE;
    } /* end of copy_setup() */

/*
** Copy text between the end of the document setup section and
** the start of the 1st page.  This could be important in an EPS
** file which contains no pages.
** Return -1 if we hit "%%Trailer" or end of file
*/
static int copy_nonpages(void)
    {
    do  {
	if(line[0] == '%' && level == 0)
	    {
	    if(strcmp(line, "%%Trailer") == 0)
		return -1;
	    if(lmatch(line, "%%Page:"))
		return 0;
	    }

	printer_write(line, line_len);
	if(!line_overflow)
	    printer_putc('\n');
	} while(dgetline(text));

    return -1;
    } /* end of copy_nonpages() */

/*--------------------------------------------------------------------
** Copy the pages to the output file.  The principal function,
** copy_pages() is called with a %%Page line already in the buffer.
--------------------------------------------------------------------*/

/* Get "-pages" file line. */
static char *getpline(void)
    {
    int len;

    if(!fgets(pline, sizeof(pline), page_comments))
	return (char*)NULL;

    /* Remove linefeed. */
    len = strlen(pline);
    if(len)
	pline[len-1] = '\0';

    return pline;
    } /* end of getpline() */

/*
** Make a table of the offset of each page record in the "-pages" file.
** This routine leaves the pointer of the pages file at the begining
** of the line which gives the offset of the start of the trailer.
*/
static int make_pagetable(void)
    {
    const char function[] = "make_pagetable";
    int available_pages, printed_pages, page, index;
    long int temp_offset;
    int direction;

    /* If nothing to do, get out. */
    if(job.attr.pages <= 0)
    	return 0;

    /* How many page descriptions are there in the PostScript code? */
    available_pages = job.attr.pages;

    /* How many of those will we be printing? */
    printed_pages = pagemask_count(&job);

    /* Allocate memory to hold the table of pages. */
    pagetable = gu_alloc(printed_pages, sizeof(struct PAGETABLE));

    /* Which order are the page descriptions in? */
    if(job.attr.pageorder == PAGEORDER_DESCEND)
    	{
    	direction = -1;
    	page = available_pages;
    	}
    else
    	{
    	direction = 1;
    	page = 1;
    	}

    /* Stop thru the list, adding the pages to be printed to the page table. */
    for(index = 0; page >= 1 && page <= available_pages; page += direction)
	{
	if(!getpline())			/* read "%%Page:" line */
	    fatal(EXIT_JOBERR, "%s(): bad job: EOF where \"%%%%Page:\" expected", function);

	if(!lmatch(pline, "%%Page:"))
	    fatal(EXIT_JOBERR, "%s(): bad job: found \"%s\" where \"%%%%Page:\" expected", function, pline);

	if(!getpline())			/* read "Offset:" line */
	    fatal(EXIT_JOBERR, "%s(): bad job: EOF where \"Offset:\" expected", function);

	if(sscanf(pline, "Offset: %ld", &temp_offset) != 1)
	    fatal(EXIT_JOBERR, "%s(): bad job: found \"%s\" where \"Offset:\" expected", function, pline);

	/* If this one should be printed, */
	if(pagemask_get_bit(&job, page) == 1)
	    {
	    if(index >= printed_pages)
	    	fatal(EXIT_JOBERR, "%s(): bad job: pagetable[] overflow", function);
	    pagetable[index].pages_offset = ftell(page_comments);
	    pagetable[index].text_offset = temp_offset;
	    index++;
	    }

	/* Move ahead to the next page in the -pages file.  This
	   involves reading up to the next blank line. */
	while(TRUE)
	    {
	    if(!getpline())
		fatal(EXIT_JOBERR, "%s(): invalid job: EOF before end of page in -pages file", function);
	    if(*pline == '\0')
		break;
	    }
	}

    if(index != printed_pages)
    	fatal(EXIT_JOBERR, "%s(): assertion failed, index=%d, printed_pages=%d", function, index, printed_pages);

    return index;
    } /* end of make_pagetable() */

/*
** This is called from copy_pages().
**
** Copy from the "-text" and "-pages" files to the interface pipe until the
** start of the next page or the start or the trailer is encountered.
** The "-text" and "-pages" files must be at the correct possitions
** before this routine is called.  The "-text" file must be at the start of
** the "%%Page:" comment, the "-pages" file must be just after the
** "Offset:" comment.
**
** The parameters "newnumber" is used to replace the ordinal number
** in the "%%Page:" comment.
*/
static void copy_a_page(int newnumber)
    {
    char pagemedia[MAX_MEDIANAME+1];
    char *ptr;

    pagemedia[0] = '\0';

    /* Copy the line that says "%%Page:" from the "-text" file to
       the output.  (Actually, we don't precisely copy it, we read
       it, parse it, and print a new "%%Page:" line with a new
       ordinal page number.)
       */
    dgetline(text);
    tokenize();
    printer_printf("%%%%Page: %s %d\n", tokens[1] ? tokens[1] : "?", newnumber);
    progress_page_start_comment_sent();		/* tell routines in pprdrv_progress.c */

    /* Copy comments from "-pages" file until blank line. */
    while(getpline() && *pline)
	{
	printer_printf("%s\n", pline);
	gu_sscanf(pline, "%%%%PageMedia: %#s", sizeof(pagemedia), pagemedia);
	}

    /* Copy blank lines before "%%BeginPageSetup". */
    while((ptr = dgetline(text)) && line[0] == '\0')
    	printer_putc('\n');

    /* Copy the "%%BeginPageSetup" line if it is there. */
    if(strncmp(line, "%%BeginPageSetup", 15) == 0)
    	{
    	printer_putline(line);
    	ptr = dgetline(text);
    	}

    /* We may want to reset the jobtimeout. */
    insert_userparams_jobtimeout();

    /* We may want to insert bin selection code here. */
    #ifdef DEBUG_BINSELECT_INLINE
    printer_printf("%% media_count=%d, job.opts.binselect=%d, pagemedia=\"%s\", default_pagemedia=\"%s\"\n",media_count,job.opts.binselect,pagemedia,default_pagemedia);
    #endif
    if(media_count > 1 && job.opts.binselect && (pagemedia[0] != (char)NULL || default_pagemedia[0] != (char)NULL))
	{
	/*
	** If we manage to select the media, strip_binselects will
	** be set to TRUE to that the old bin select code will be
	** removed.  If this printer does not have bins defined, this
	** function call will return FALSE.
	*/
	if(pagemedia[0] != '\0')
	    strip_binselects = select_medium_by_dsc_name(pagemedia);
	else
	    strip_binselects = select_medium_by_dsc_name(default_pagemedia);
	}

    /* Copy rest of page text from "-text" file. */
    while(ptr && (level || line[0] != '%' ||
		(strncmp(line, "%%Page:", 7) && strcmp(line, "%%Trailer")) ) )
	{
	printer_write(line, line_len);		/* Copy the line to the printer. */

	if(!line_overflow)			/* If it was a whole line, */
	    printer_putc('\n');			/* terminate it. */

	ptr = dgetline(text);
	}

    strip_binselects = FALSE;
    } /* end of copy_a_page() */

/*
** Called from main, this ties together a number of routines such as
** make_pagetable() and copy_a_page().
**
** What does this mean?:
** In this routine, we number the pages from 0 to npages-1.
*/
static void copy_pages(void)
    {
    const char function[] = "copy_pages";
    long int trailer_offset_pages;	/* used to save position in -pages */
    int sheetnumber;			/* cardinal current sheet number */
    int sheetlimit;			/* page number at which we stop */
    int sheetmember;			/* index into current sheet (cardinal) */
    int npages;				/* Number of pages in document */
    int pagenumber;			/* cardinal number of current page */
    int pageindex;			/* array index of printing page */

    /* Read in a list of pages: */
    npages = make_pagetable();

    /* We can't copy pages if they don't exist: */
    if(npages == 0)
	return;

    /*
    ** Remember the offset of the start of the trailer in the "-pages" file.
    ** We do this so that we can move the file pointer back when this
    ** function is done.
    */
    trailer_offset_pages = ftell(page_comments);

    /*
    ** This is the start of a loop which is used to emmit multiple copies
    ** of the script section of a DSC commented PostScript file.  This might
    ** be done to produce collated copies.
    */
    while(copies_pages_countdown--)
	{
    	if(print_direction == -1)	/* If printing backwards, */
	    {
	    sheetnumber = sheetcount-1;	/* start with last sheet */
	    sheetlimit = -1;		/* and end when we overshoot the 1st */
	    }
    	else				/* If printing forward, */
	    {
	    sheetnumber = 0;		/* start with 1st sheet */
	    sheetlimit = sheetcount;	/* and end when we overshoot last */
	    }

    	/*
    	** We go thru this loop once for each sheet.
    	** We will either start at the first sheet
    	** and work toward the end of the document
    	** or start at the end and work toward the
    	** begining.
    	*/
    	while(sheetnumber != sheetlimit)
      	    {
	    #ifdef DEBUG_SIGNITURE_INLINE
	    printer_printf("%% sheetnumber = %d\n", sheetnumber);
	    #endif

	    #ifdef DELETED_CODE
	    /*
	    ** If the -p switch was used it could be that we
	    ** are supposed to skip this sheet.
	    ** The evenness and oddness tests look backwards
	    ** but they are not.  They look backwards because
	    ** sheetnumber is a cardinal number.
	    ** Both job.portion.start and job.portion.stop will
	    ** be equal to zero if there is no start or stop limit
	    ** respectively.
	    */
	    if(    ( (sheetnumber%2 == 0) && !job.portion.odd )
	    	|| ( (sheetnumber%2 == 1) && !job.portion.even )
	    	|| ( job.portion.start && sheetnumber < (job.portion.start-1) )
		|| ( job.portion.stop && sheetnumber > (job.portion.stop-1) )
			)
		{
		#ifdef DEBUG_SIGNITURE_INLINE
		printer_printf("%% skiping this sheet\n");
		#endif
		sheetnumber += print_direction;
		continue;
		}
	    #endif

	    /*
	    ** We go thru this loop once for each virtual page on
	    ** the sheet.
	    */
	    for(sheetmember = 0; sheetmember < job.attr.pagefactor; sheetmember++)
		{
		#ifdef DEBUG_SIGNITURE_INLINE
		printer_printf("%%  sheetmember = %d", sheetmember);
		#endif
		/*
		** Compute the page number of the virtual page to print
		** next.  If we are not doing signiture printing this
		** is a straightforward operation.
		**
		** The number we compute is the logical page number
		** with pages numbered from 0.  If the pages appear
		** backwards in the file 0 will be the last page
		** in the file.
		*/
		if(job.N_Up.sigsheets == 0)
		    {
		    pagenumber = sheetnumber * job.attr.pagefactor + sheetmember;
		    }
		else			/* If not printing this side, */
		    {			/* (-s fonts or -s backs) */
		    if((pagenumber = signature(sheetnumber, sheetmember)) == -1)
			{
			#ifdef DEBUG_SIGNITURE_INLINE
			printer_printf(", skipping\n");
			#endif
		    	continue;	/* skip this page. */
		    	}
		    }

		/* If the pages in the file are in ascending order,
		   then the pages index into the file is equal to
		   its number. */
		if(current_job_direction == 1)
	    	    pageindex = pagenumber;

		/* If the pages are in reverse order in the file,
		   then the computation is more complicated. */
		else
		    pageindex = npages - pagenumber - 1;

		#ifdef DEBUG_SIGNITURE_INLINE
		printer_printf(", pagenumber = %d, pageindex = %d\n", pagenumber, pageindex);
		#endif

		if(pageindex < npages && pageindex >= 0)	/* If we have such a page, */
	    	    {
	    	    if(fseek(page_comments, pagetable[pageindex].pages_offset, SEEK_SET))
			fatal(EXIT_JOBERR, "%s(): fseek() error (-pages)", function);

		    if(fseek(text, pagetable[pageindex].text_offset, SEEK_SET))
			fatal(EXIT_JOBERR, "%s(): fseek() error (-text)", function);

		    copy_a_page(pagenumber + 1);	/* copy this one page */
		    }
		else                              /* No such page, print a dummy page */
	    	    {                             /* if not last sheet to be emmited */
	    	    if( copies_pages_countdown	  /* and not last copy */
		    	|| job.N_Up.sigsheets
		    	    || ((sheetnumber+print_direction) != sheetlimit) )
			{
			printer_printf("%%%%Page: dummy %d\n", pagenumber + 1);
			printer_puts("showpage\n\n");	/* note extra newline */
		    	}
	    	    }
	    	} /* end of page loop */
	    sheetnumber += print_direction;       /* compute next sheet number */
      	    } /* end of sheet loop */
    	} /* end of the pages multiple copies loop */

    /*
    ** Take up position at the start of the document trailer which
    ** is where one would expect the -text and -pages file pointers to be
    ** after copying all the pages.
    */
    if(fseek(page_comments, trailer_offset_pages, SEEK_SET))
	fatal(EXIT_JOBERR, "%s(): can't seek to trailer in -pages", function);
    } /* end of copy_pages() */

/*
** Copy the trailer to the output file.
** Unless we hit end of file or "%%EOF", the line "%%Trailer"
** should be in the buffer.
*/
static void copy_trailer(void)
    {
    const char function[] = "copy_trailer";
    long int temp_offset;

    /* Read the location of the trailer from the -pages file and seek to that
       location in the -text file. */
    if(!getpline())
    	fatal(EXIT_JOBERR, "%s(): bad job: got EOF when \"%%%%Trailer\" expected", function);
    if(strcmp(pline, "%%Trailer") != 0)
    	fatal(EXIT_JOBERR, "%s(): bad job: got \"%s\" when \"%%%%Trailer\" expected", function, pline);
    if(!getpline())
    	fatal(EXIT_JOBERR, "%s(): bad job: got EOF when \"Offset:\" expected", function);
    if(sscanf(pline, "Offset: %ld", &temp_offset) != 1)
    	fatal(EXIT_JOBERR, "%s(): bad job: got \"%s\" when \"Offset:\" expected", function, pline);
    if(fseek(text, temp_offset, SEEK_SET))
    	fatal(EXIT_JOBERR, "%s(): can't seek to trailer in -text", function);

    /* Copy -text to printer until EOF or unenclosed %%EOF comment. */
    while(dgetline(text))
	{
	/* Write, specifying length in case of embeded nulls. */
	printer_write(line, line_len);

	/* If line was not a partial one, terminate it. */
	if(!line_overflow)
	    printer_putc('\n');

	/* If we see our "%%EOF", then stop. */
	if(level==0 && strcmp(line, "%%EOF") == 0)
	    break;
	}

    /* If N-Up was invoked, clean up. */
    close_N_Up();

    printer_puts("%%EOF\n");
    } /* end of copy_trailer() */

/*================================================================
** End of job reasembly code,
** Start of helper routines for main().
================================================================*/

/*
** Read the parts of the printer configuration that we need.
** We must get the name of the interface, the address, and the options.
*/
static void pprdrv_read_printer_conf(void)
    {
    char cfname[MAX_PPR_PATH];		/* name of printer conf file */
    FILE *cfstream;			/* stream of printer conf file */
    int linenum = 0;			/* current line number */
    char *confline = NULL;		/* current line */
    int confline_len = 128;		/* current size of confline[] array */

    float tf1, tf2;			/* temporary values for use with gu_sscanf() */
    char *tptr;				/* temporary value for use with gu_sscanf() */
    int tint;				/* temporary value for use with gu_sscanf() */
    char scratch[10];			/* temporary storage for use with gu_sscanf() */
    int count;				/* for gu_sscanf() return value */

    gu_boolean saw_feedback, saw_jobbreak, saw_codes;

    /* Construct the path to the printer configuration file and open it. */
    ppr_fnamef(cfname, "%s/%s", PRCONF, printer.Name);
    if((cfstream = fopen(cfname, "r")) == (FILE*)NULL)
	fatal(EXIT_PRNERR, "can't open printer config file \"%s\", errno=%d (%s)", cfname, errno, gu_strerror(errno));

    printer.Interface = NULL;
    printer.Address = NULL;
    printer.Options = NULL;
    printer.RIP.name = NULL;
    printer.RIP.output_language = NULL;
    printer.RIP.options_storage = NULL;
    printer.Commentators = (struct COMMENTATOR *)NULL;
    printer.do_banner = BANNER_DISCOURAGED;	/* default flag */
    printer.do_trailer = BANNER_DISCOURAGED;	/* page settings */
    printer.OutputOrder = 0;			/* unknown */
    printer.charge.per_duplex = 0;		/* default: no charge (not same as zero charge) */
    printer.charge.per_simplex = 0;
    printer.type42_ok = FALSE;			/* paranoid default */
    printer.GrayOK = TRUE;			/* most printers allow non-colour jobs */
    printer.PageCountQuery = 0;			/* don't query */
    printer.PageTimeLimit = 0;
    printer.limit_pages.lower = 0;
    printer.limit_pages.upper = 0;
    printer.limit_kilobytes.lower = 0;
    printer.limit_kilobytes.upper = 0;
    printer.custom_hook.flags = 0;
    printer.custom_hook.path = NULL;
    printer.userparams.JobTimeout = -1;
    printer.userparams.WaitTimeout = -1;
    printer.userparams.ManualfeedTimeout = -1;
    printer.userparams.DoPrintErrors = -1;

    saw_feedback = saw_jobbreak = saw_codes = FALSE;

    /* Read the printer configuration file line-by-line. */
    while((confline = gu_getline(confline, &confline_len, cfstream)))
	{
	linenum++;		/* current line number (for error messages) */

	if(*confline==';' || *confline=='#')  /* ignore comments */
	    continue;

	/* Read name of interface program. */
	if(gu_sscanf(confline, "Interface: %S", &tptr) == 1)
	    {
	    if(printer.Interface) gu_free(printer.Interface);
	    printer.Interface = tptr;
	    saw_feedback = saw_jobbreak = saw_codes = FALSE;
	    }

	/*
	** Read address to use with interface.  An address may contain
	** embedded spaces.  It may also be quoted to allow leading
	** or trailing spaces.
	*/
	else if(gu_sscanf(confline, "Address: %A", &tptr) == 1)
	    {
	    if(printer.Address) gu_free(printer.Address);
	    printer.Address = tptr;
	    }

	/* read the feedback setting */
	else if((tptr = lmatchp(confline, "Feedback:")))
	    {
	    if(gu_torf_setBOOL(&printer.Feedback, tptr))
		fatal(EXIT_PRNERR_NORETRY, _("Invalid \"%s\" (%s line %d)."), "Feedback:", cfname, linenum);
	    saw_feedback = TRUE;
	    }

	/* Read control-d use setting. */
	else if(gu_sscanf(confline, "JobBreak: %d", &printer.Jobbreak) == 1)
	    {
	    saw_jobbreak = TRUE;
	    }

	/* Read passable codes setting. */
	else if(gu_sscanf(confline, "Codes: %d", &tint) == 1)
	    {
	    switch(tint)
	    	{
		case CODES_UNKNOWN:
		case CODES_Clean7Bit:
		case CODES_Clean8Bit:
		case CODES_Binary:
		case CODES_TBCP:
		    printer.Codes = (enum CODES)tint;
		    break;
		default:
		    fatal(EXIT_PRNERR_NORETRY, _("Invalid \"%s\" (%s line %d)."), "Codes:", cfname, linenum);
		    break;
	    	}
	    saw_codes = TRUE;
	    }

	/*
	** Read options to use with interface.  Take the whole rest of the line.
	*/
	else if(gu_sscanf(confline, "Options: %Z", &tptr) == 1)
	    {
	    if(printer.Options) gu_free(printer.Options);
	    printer.Options = tptr;
	    }

	/*
	** Read the Raster Image Processor settings.
	*/
	else if((tptr = lmatchp(confline, "RIP:")))
	    {
	    if(printer.RIP.name)
	    	gu_free(printer.RIP.name);
	    if(printer.RIP.output_language)
	    	gu_free(printer.RIP.output_language);
	    if(printer.RIP.options_storage)
	    	gu_free(printer.RIP.options_storage);
	    if((count = gu_sscanf(tptr, "%S %S %Z", &printer.RIP.name, &printer.RIP.output_language, &printer.RIP.options_storage)) < 3)
	    	fatal(EXIT_PRNERR_NORETRY, "Invalid \"%s\" (%s line %d).", "RIP:", cfname, linenum);
	    }

	/*
	** Read amount to charge the poor user.
	** We read two amounts in dollars or pounds, etc.
	** and then convert them to cents, new pence, etc.
	*/
	else if((count = sscanf(confline, "Charge: %f %f", &tf1, &tf2)) >= 1)
	    {   /* ^ notice use of sscanf() rather than gu_sscanf() ^ */
	    printer.charge.per_duplex = (int)(tf1 * 100.0 + 0.5);
	    if(count == 2)
	    	printer.charge.per_simplex = (int)(tf2 * 100.0 + 0.5);
	    else
	    	printer.charge.per_simplex = printer.charge.per_duplex;
	    }

	/* Read the printer's flag page vote. */
	else if((count = gu_sscanf(confline, "FlagPages: %d %d", &printer.do_banner, &printer.do_trailer)) > 0)
	    {
	    if(count != 2 || printer.do_banner > BANNER_REQUIRED
		    || printer.do_banner < BANNER_FORBIDDEN
		    || printer.do_trailer > BANNER_REQUIRED
		    || printer.do_trailer < BANNER_FORBIDDEN )
		fatal(EXIT_PRNERR_NORETRY, "Invalid \"%s\" (%s line %d).", "FlagPages:", cfname, linenum);
	    }

	/*
	** Read the printers OutputOrder specification.
	** Remember that if there is no such line, the
	** information is taken from the PPD file.
	*/
	else if(gu_sscanf(confline, "OutputOrder: %#s", sizeof(scratch), scratch) == 1)
	    {
	    if(strcmp(scratch, "Normal") == 0)
		printer.OutputOrder = 1;
	    else if(strcmp(scratch, "Reverse") == 0)
		printer.OutputOrder = -1;
	    else
		fatal(EXIT_PRNERR_NORETRY, _("Invalid \"%s\" (%s line %d)."), "OutputOrder:", cfname, linenum);
	    }

	else if(gu_sscanf(confline, "PPDFile: %A", &tptr) == 1)
	    {
	    if(printer.PPDFile) gu_free(printer.PPDFile);
	    if(tptr[0] == '/')			/* use absolute path */
		{				/* directly */
		printer.PPDFile = tptr;
		}
	    else				/* prepend standard PPD file directory */
	        {				/* to relative paths */
		char fname[MAX_PPR_PATH];
		ppr_fnamef(fname, "%s/%s", PPDDIR, tptr);
		gu_free(tptr);
		printer.PPDFile = gu_strdup(fname);
	        }
	    }

	else if(lmatch(confline, "Commentator:"))
	    {
	    struct COMMENTATOR *newcom;

	    newcom = (struct COMMENTATOR*)gu_alloc(1,sizeof(struct COMMENTATOR));
	    newcom->options = (char*)NULL;

	    if(gu_sscanf(confline, "Commentator: %d %Q %Q %Q",
	    		&newcom->interests,		/* interesting commentary() flags values */
	    		&newcom->progname,		/* commentator program */
	    		&newcom->address,		/* address */
			&newcom->options) >= 3)
	    	{
		newcom->next = printer.Commentators;
		printer.Commentators = newcom;
	    	}
	    else
	    	{
		fatal(EXIT_PRNERR_NORETRY, _("Invalid \"%s\" (%s line %d)."), "Commentator:", cfname, linenum);
		}
	    }
	else if((tptr = lmatchp(confline, "GrayOK:")))
	    {
	    if(gu_torf_setBOOL(&printer.GrayOK, tptr) == -1)
	    	fatal(EXIT_PRNERR_NORETRY, _("Invalid \"%s\" (%s line %d)."), "GrayOK:", cfname, linenum);
	    }
	else if((count = gu_sscanf(confline, "LimitPages: %d %d", &printer.limit_pages.lower, &printer.limit_pages.upper)) > 0)
	    {
	    if(count != 2 || printer.limit_pages.lower < 0 || printer.limit_pages.upper < printer.limit_pages.lower)
		fatal(EXIT_PRNERR_NORETRY, _("Invalid \"%s\" (%s line %d)."), "LimitPages:", cfname, linenum);
	    }
	else if((count = gu_sscanf(confline, "LimitKilobytes: %d %d", &printer.limit_kilobytes.lower, &printer.limit_kilobytes.upper)) > 0)
	    {
	    if(count != 2 || printer.limit_kilobytes.lower < 0 || printer.limit_kilobytes.upper < printer.limit_kilobytes.lower)
		fatal(EXIT_PRNERR_NORETRY, _("Invalid \"%s\" (%s line %d)."), "LimitKilobytes:", cfname, linenum);
	    }
	else if(gu_sscanf(confline, "PageCountQuery: %d", &printer.PageCountQuery) == 1)
	    {
	    /* nothing more to do here */
	    }
	else if(gu_sscanf(confline, "PageTimeLimit: %d", &printer.PageTimeLimit) == 1)
	    {
	    /* nothing more to do here */
	    }
	else if((count = gu_sscanf(confline, "CustomHook: %d %Q", &printer.custom_hook.flags, &tptr)) > 0)
	    {
	    if(count != 2)
		fatal(EXIT_PRNERR_NORETRY, _("Invalid \"%s\" (%s line %d)."), "CustomHook:", cfname, linenum);
	    if(printer.custom_hook.path) gu_free((char*)printer.custom_hook.path);
	    printer.custom_hook.path = tptr;
	    }
	else if((tptr = lmatchp(confline, "Userparams:")))
	    {
	    char *ptr_parse = tptr;
	    char *ptr_pair;
	    char *ptr_name;
	    char *ptr_value;

	    while((ptr_pair = gu_strsep(&ptr_parse, " ")))
	    	{
		if(!(ptr_name = gu_strsep(&ptr_pair, "="))
			|| !(ptr_value = gu_strsep(&ptr_pair, "="))
                        || ptr_name[0] == '\0'
                        || ptr_value[0] == '\0')
		    fatal(EXIT_PRNERR_NORETRY, _("Syntax error in \"%s\" (%s line %d)."), "Userparams:", cfname, linenum);

		if(gu_strcasecmp(ptr_name, "JobTimeout") == 0)
		    printer.userparams.JobTimeout = atoi(ptr_value);
		else if(gu_strcasecmp(ptr_name, "WaitTimeout") == 0)
		    printer.userparams.WaitTimeout = atoi(ptr_value);
		else if(gu_strcasecmp(ptr_name, "ManualfeedTimeout") == 0)
		    printer.userparams.ManualfeedTimeout = atoi(ptr_value);
		else if(gu_strcasecmp(ptr_name, "DoPrintErrors") == 0)
		    {
		    gu_boolean answer;
		    if(gu_torf_setBOOL(&answer, ptr_value) == -1)
		    	fatal(EXIT_PRNERR_NORETRY, _("Value of \"%s\" in \"%s\" must be boolean."), "DoPrintErrors", "Userparams:");
		    printer.userparams.DoPrintErrors = answer ? 1 : 0;
		    }
		else
		    {
		    fatal(EXIT_PRNERR_NORETRY, _("Unrecognized userparams variable \"%s\"."), ptr_name);
		    }
	    	}
	    }

	} /* end of while()  Unknown lines are ignored
	     because pprdrv doesn't use all possible lines. */

    /* Close the printer configuration file. */
    fclose(cfstream);

    /* An interface name is the only mandatory configuration element! */
    if(!printer.Interface)
    	fatal(EXIT_PRNERR_NORETRY, _("No \"Interface:\" line in %s"), cfname);

    if(!printer.Address)
        printer.Address = printer.Name;

    if(!printer.Options)
        printer.Options = "";

    /* Use a Flex scanner to read information from the PPD file and store
       it in the printer structure. */
    read_PPD_file(printer.PPDFile);
    DODEBUG_PPD(("after reading PPD: printer.prot.PJL=%s, printer.prot.TBCP=%s", printer.prot.PJL ? "TRUE" : "FALSE", printer.prot.TBCP ? "TRUE" : "FALSE"));

    /* If these things weren't set in the printer's configuration file,
       then set them to the default values.  The default values depend
       on which interface is being used and what the PPD files has
       to say. */
    if(! saw_feedback)
    	printer.Feedback = interface_default_feedback(printer.Interface, &printer.prot);
    if(! saw_jobbreak)
    	printer.Jobbreak = interface_default_jobbreak(printer.Interface, &printer.prot);
    if(! saw_codes)
	printer.Codes = interface_default_codes(printer.Interface, &printer.prot);

    /*
    ** Now we have to parse the RIP options.  It would be nice if we had a
    ** library function for this.
    */
    printer.RIP.options_count = 0;
    if(printer.RIP.options_storage)
	{
	char *si, *di;

	/* Estimate how many items there are, erring on the side of caution. */
	{
	int count;
	for(count = 1, si = printer.RIP.options_storage; *si; si++)
	    {
	    if(isspace(*si))
	    	count++;
	    }
	printer.RIP.options = gu_alloc(count, sizeof(const char *));
	}

	/* Parse into "words" with crude quoting. */
	si = di = printer.RIP.options_storage;
	while(*si)
	    {
	    si += strspn(si, " \t");	/* skip whitespace */
	    if(*si)			/* if anything left, */
		{
		int c, quote = 0;
		printer.RIP.options[printer.RIP.options_count++] = di;
		while((c = *si))
		    {
		    si++;		/* Mustn't this until after we know it isn't a NULL! */
		    switch(c)
		    	{
			case '\"':
			case '\'':
			    if(c == quote)
			    	quote = 0;
			    else if(!quote)
				quote = c;
			    else
			    	*di++ = c;
			    break;

			case ' ':
			case '\t':
			    if(!quote)
			    	goto break_break;
			    *di++ = c;
			    break;

			default:
			    *di++ = c;
			    break;
		    	}
		    }
		break_break:
		*di++ = '\0';
		/* debug("RIP option: \"%s\"", printer.RIP.options[printer.RIP.options_count - 1]); */
		}
	    }

	printer.RIP.options = gu_realloc(printer.RIP.options, printer.RIP.options_count, sizeof(const char *));
	}

    } /* end of pprdrv_read_printer_conf() */

/*
** Choose the method by which we will print multiple copies.
** This routine will always return.  It is called once by main().
*/
static void select_copies_method(void)
    {
    if(job.opts.copies < 2)
	{
	copies_auto = FALSE;
	copies_doc_countdown = 1;
	copies_pages_countdown = 1;
	return;
	}

    if(!job.opts.collate)
	{
	copies_auto = TRUE;
	copies_doc_countdown = 1;
	copies_pages_countdown = 1;
	}
    else
	{
	if( !job.attr.script || (job.attr.pageorder==PAGEORDER_SPECIAL) )
	    {
	    copies_auto = FALSE;
	    copies_doc_countdown = job.opts.copies;
	    copies_pages_countdown = 1;
	    }
	else
	    {
	    copies_auto = FALSE;
	    copies_doc_countdown = 1;
	    copies_pages_countdown = job.opts.copies;
	    }
	}
    } /* end of select_copies_method() */

/*
** Compute things about pages.  This is called once from main().
** This routine will only exit if there is an internal error.
*/
static void page_computations(void)
    {
    int pages = pagemask_count(&job);

    /*
    ** Compute the number of physical pages in one copy of this job.
    **
    ** job.N_Up.sigsheets is the number of pieces of paper in each
    ** signiture.
    **
    ** job.attr.pages is the number of "%%Page:" comments in the file.  However
    ** pages contains the number of those pages we are going to print.
    **
    ** job.attr.pagefactor is the number of "%%Page:" comments per
    ** piece of paper.  It is the N-Up factor times 1 for simplex
    ** or 2 for duplex.
    */
    if(job.N_Up.sigsheets == 0)		/* if not signature printing, */
	{
    	sheetcount = (pages + job.attr.pagefactor - 1) / job.attr.pagefactor;
	}
    else				/* if signature printing, */
    	{
	/* Compute number of signatures required, counting any partial sig as whole */
	int signature_count=
		(pages+(job.N_Up.N * 2 * job.N_Up.sigsheets)-1)
			/
		(job.N_Up.N * 2 * job.N_Up.sigsheets);

    	/* Now that we know the number of signitures, compute the number of
    	   sheets.   The clause (job.N_Up.N * 2)/job.attr.pagefactor) doubles
    	   the number of sheets if duplex mode is off. */
	sheetcount = signature_count * job.N_Up.sigsheets * ((job.N_Up.N * 2)/job.attr.pagefactor);
    	}

    /*
    ** Determine which way the pages are ordered now.
    */
    if(job.attr.pageorder == PAGEORDER_SPECIAL)
	current_job_direction = 1;	/* assume it is not backwards */
    else if(job.attr.pageorder == PAGEORDER_ASCEND)
	current_job_direction = 1;	/* it is forward */
    else if(job.attr.pageorder == PAGEORDER_DESCEND)
	current_job_direction = -1;	/* it is backwards */
    else
	fatal(EXIT_JOBERR, "PageOrder is invalid");

    /*
    ** If changing the page order is permissible, then reverse
    ** the printing direction if we believe this printer has
    ** a face up output tray.
    */
    if(job.attr.pageorder == PAGEORDER_SPECIAL)
    	print_direction = 1;
    else
	print_direction = printer.OutputOrder;

    DODEBUG_MAIN(("page_computations(): page_computations(): pageorder=%d, OutputOrder=%d, print_direction=%d, current_job_direction=%d",
	job.attr.pageorder, printer.OutputOrder, print_direction, current_job_direction));
    } /* end of page_computations() */

/*
** If we suceeded in printing the document and a printlog
** file exists, put an entry in it.   This routine
** is called once by main().
*/
static void printer_use_log(struct timeval *start_time, int pagecount_start, int pagecount_change)
    {
    const char function[] = "printer_use_log";
    int printlogfd;

    if((printlogfd = open(PRINTLOG_PATH, O_WRONLY | O_APPEND)) >= 0)
	{
	FILE *printlog;			/* For print log file. */

	if((printlog = fdopen(printlogfd, "a")) == (FILE*)NULL)
	    {
	    error("%s(): fdopen() failed", function);
	    close(printlogfd);
	    }
	else
	    {
	    int sidecount;
	    int total_printed_sheets;
	    int total_printed_sides;

	    struct timeval time_now, time_elapsed;
	    time_t seconds_now; 		/* For time stamping the */
	    struct tm *time_detail;		/* print log file. */
	    char time_str[15];

	    int pages = pagemask_count(&job);
	    struct COMPUTED_CHARGE charge;

	    /*
	    ** Compute the number of printed sides.  Normally this is
	    ** straight-forward, but in signiture mode we must account
	    ** for "pages" that are left blank.
	    */
	    if(job.N_Up.sigsheets == 0)
	    	{
	    	sidecount = (pages + job.N_Up.N - 1) / job.N_Up.N;
	    	}
	    else
	    	{
		if((job.N_Up.sigpart & SIG_BOTH) == SIG_BOTH)
	    	    sidecount = sheetcount * 2;			/* !!! */
		else
	    	    sidecount = sheetcount;			/* !!! */
		}

	    /*
	    ** Compute the total number of sheets and the total number of printed sides
	    ** in all copies of this job.  Remember that if the number of copies desired
	    ** was not specified then job.opts.copies will be -1.  In this case
	    ** we will assume that the PostScript code of the job only prints one copy.
	    */
	    if(job.opts.copies > 1)		/* if multiple copies */
		{
		total_printed_sheets = sheetcount * job.opts.copies;	/* !!! */
		total_printed_sides = sidecount * job.opts.copies;
		}
	    else				/* 1 or unknown # of copies */
		{
		total_printed_sheets = sheetcount;			/* !!! */
		total_printed_sides = sidecount;
		}

	    /*
	    ** Get the current time with microsecond resolution
	    ** and compute the difference between the current time
	    ** and the time that was recorded when we started.
	    */
	    gettimeofday(&time_now, (struct timezone *)NULL);
	    time_elapsed.tv_sec = time_now.tv_sec - start_time->tv_sec;
	    time_elapsed.tv_usec = time_now.tv_usec - start_time->tv_usec;
	    if(time_elapsed.tv_usec < 0) {time_elapsed.tv_usec += 1000000; time_elapsed.tv_sec--;}

	    /* Convert the current time to a string of ASCII digits. */
	    seconds_now = /* time((time_t*)NULL) */ time_now.tv_sec;
	    time_detail = localtime(&seconds_now);
	    strftime(time_str, sizeof(time_str), "%Y%m%d%H%M%S", time_detail);

	    /* Compute the amount of money charged for this job. */
            charge.total = 0;
            if(printer.charge.per_duplex > 0 || printer.charge.per_simplex > 0)
		{
		compute_charge(&charge,
			printer.charge.per_duplex,
			printer.charge.per_simplex,
			pages,
			job.N_Up.N,
			job.attr.pagefactor,
			job.N_Up.sigsheets,
			job.N_Up.sigpart,
			job.opts.copies);
		}

	    /* Print it all as a line. */
	    fprintf(printlog, "%s,%s,%s,\"%s\",%s,\"%s\",%d,%d,%d,%ld,%ld.%02d,%d.%02d,%d,%d,%d,%ld,%ld,\"%s\"\n",
	    	time_str,
	    	QueueFile,
	    	printer.Name,
		job.For ? job.For : "???",
		job.username,
		job.proxy_for ? job.proxy_for : "",
		pages,
	    	total_printed_sheets,
	    	total_printed_sides,
	    	(long)(seconds_now - job.time),
	    	(long)time_elapsed.tv_sec, (int)(time_elapsed.tv_usec / 10000),
		charge.total / 10, abs(charge.total % 10),
		feedback_pjl_chargable_pagecount(),
		pagecount_start, pagecount_change,
		job.attr.postscript_bytes,
		progress_bytes_sent_get(),
		job.Title ? job.Title : job.lpqFileName ? job.lpqFileName : ""
	    	);

	    if(fclose(printlog) == EOF)
	    	error("%s(): fclose() failed", function);

	    } /* fdopen() didn't fail */
	}
    } /* end of printer_use_log() */

/*
** Copy function for the transparent mode hack.
*/
static void transparent_hack_or_passthru_copy(const char *qf, gu_boolean is_transparent)
    {
    const char function[] = "transparent_hack_or_passthru_copy";
    char fname[MAX_PPR_PATH];
    int handle;
    char buffer[4096];
    int len;

    ppr_fnamef(fname, "%s/%s-%s", DATADIR, qf, is_transparent ? "infile" : "barbar");

    if((handle = open(fname, O_RDONLY)) == -1)
	fatal(EXIT_JOBERR, "%s(): can't open \"%s\", errno=%d (%s)", function, fname, errno, gu_strerror(errno));

    gu_set_cloexec(handle);

    job_start(is_transparent ? JOBTYPE_THEJOB_TRANSPARENT : JOBTYPE_THEJOB_BARBAR);

    while((len = read(handle, buffer, sizeof(buffer))) > 0)
	printer_write(buffer, len);

    if(len == -1)
    	fatal(EXIT_PRNERR_NORETRY, "%s(): read() failed, errno=%d (%s)", function, errno, gu_strerror(errno));

    close(handle);

    job_end();
    } /* end of transparent_hack_or_passthru_copy() */

/*
** Catch child death signals.  This handler is in placed during those
** time when we don't expect the interface to terminate.  We ignore
** the termination of things other than the interface, such as
** commentators and custom hooks.
*/
static void sigchld_handler(int sig)
    {
    const char function[] = "sigchld_handler";
    pid_t waitid;
    int wait_status;

    /*
    ** In this loop we retrieve process exit status until there
    ** are no more children whose exit status is available.  When
    ** there are no more, we return.  If the child is the interface,
    ** we drop out of the loop and let the code below handle it.
    **
    ** Notice that it is important to test for a return value of
    ** 0 before we call interface_sigchld_hook() since it will test
    ** for a return value that is equal to intpid and it sets intpid
    ** to 0 when there is not interface running.
    */
    while(TRUE)
	{
	waitid = waitpid((pid_t)-1, &wait_status, WNOHANG);

	/* If no more children have exited or no more children exist, */
	if(waitid == 0)
	    break;

	if(waitid == -1)
	    {
	    if(errno == ECHILD)
		break;
	    if(errno == EINTR)
	    	continue;
	    signal_fatal(EXIT_PRNERR, "%s(): waitpid() failed, errno=%d (%s)", function, errno, gu_strerror(errno));
	    }

	/* If the child that died was the interface, */
	if(interface_sigchld_hook(waitid, wait_status))
	    continue;

	/* If the RIP terminated, */
	if(rip_sigchld_hook(waitid, wait_status))
	    continue;

	/* If we get this far, a commentator terminated. */
	#ifdef DEBUG_INTERFACE
	signal_debug("%s(): process %ld terminated", function, (long int)waitid);
	#endif
    	}
    } /* end of sigchld_handler() */

/*
** Handler for terminate signal.  We set a flag which causes the main loop
** to terminate.
**
** While this handler is named for SIGTERM, it is also installed as the handler
** for a number of other signals that can be reasonably interpreted as
** terminate requests.  Look in main() to see which signals.
*/
static void sigterm_handler(int sig)
    {
    FUNCTION4DEBUG("sigterm_handler");

    #ifdef DEBUG_MAIN
    signal_debug("%s(): caught signal %d (%s)", function, sig, gu_strsignal(sig));
    #endif

    /*
    ** When testing code which is invoked only if certain events occur while
    ** halting the printer or canceling the active job then delay here so that
    ** the cancel operation will take a noticable period of time.
    **
    ** We have to be ready to restart sleep() because SIGCHLD might
    ** interupt it.  We may get SIGCHLD a lot because commentators may be
    ** chattering.
    */
    #ifdef DEBUG_DIE_DELAY
    {
    int unslept = DEBUG_DIE_DELAY;
    signal_debug("%s(): delaying for %d seconds", function, DEBUG_DIE_DELAY);
    while((unslept = sleep(unslept)) > 0);
    signal_debug("%s(): delay ended", function);
    }
    #endif

    sigterm_caught = TRUE;
    } /* end of sigterm_handler() */

/*
** We use alarm() to set timeouts for certain operations.  We check the
** variable sigalrm_caught to figure out if the operation suceeded or
** if it was interupted by the SIGALRM.
*/
static void sigalrm_handler(int sig)
    {
    #ifdef DEBUG_INTERFACE
    signal_debug("SIGALRM caught");
    #endif
    sigalrm_caught = TRUE;
    }

/*
** We might catch this signal if the interface dies.  We basically ignore
** since we have other means of knowning that that interface has died.
*/
static void sigpipe_handler(int sig)
    {
    #ifdef DEBUG_INTERFACE
    signal_debug("SIGPIPE caught");
    #endif
    }

/*
** This is called periodicaly from the output code to make sure we haven't
** been asked to exit, that the interface hasn't died, and that the
** RIP (if any) hasn't died.
*/
void fault_check(void)
    {
    FUNCTION4DEBUG("fault_check")
    DODEBUG_INTERFACE(("%s()", function));

    /* Not strictly an interface fault, but we catch it here. */
    if(sigterm_caught)
	{
	DODEBUG_INTERFACE(("%s(): exiting because sigterm_caught is TRUE", function));
	hooked_exit(EXIT_SIGNAL, "printing halted");
	}

    /* Check to see if the interface has exited prematurely. */
    interface_fault_check();

    /* Check to see if the RIP (if we are using one) has exited prematurely. */
    rip_fault_check();

    DODEBUG_INTERFACE(("%s(): done", function));
    } /* end of fault_check() */

/*
** main procedure
*/
int main(int argc, char *argv[])
    {
    int result;		/* exit code of interface */
    int group_pass;	/* Number of the pass thru the group (a pprdrv invokation parameter). */
    int argi = 1;
    int pagecount_start, pagecount_change;

    /* Note the start time for the logs: */
    gettimeofday(&start_time, (struct timezone *)NULL);

    /* Initialize internation messages library. */
    #ifdef INTERNATIONAL
    setlocale(LC_MESSAGES, "");
    bindtextdomain(PACKAGE_PPRDRV, LOCALEDIR);
    textdomain(PACKAGE_PPRDRV);
    #endif

    /* Should we run in test mode? */
    if(argc >= 2 && strcmp(argv[argi], "--test") == 0)
    	{
    	argi++;
    	test_mode = TRUE;
	chdir(HOMEDIR);
    	}

    /*
    ** If fewer than 3 remaining arguments,
    ** (We mustn't call fatal() or hooked_exit() here because printer.Name is not yet set.)
    */
    if((argc - argi) < 3)
	{
	fputs("Usage: pprdrv [--test] <printer> <queuefile> <pass>\n", stderr);
	exit(EXIT_PRNERR_NORETRY);
	}

    /* Assign each of the three parameters to a variable with a meaningful name: */
    printer.Name = argv[argi];
    QueueFile = argv[argi+1];
    group_pass = atoi(argv[argi+2]);

    /* If any debugging at all is turned on, then log the program startup. */
    #ifdef DEBUG
    debug("main(): printer.Name=\"%s\", QueueFile=\"%s\", group_pass=%d", printer.Name, QueueFile, group_pass);
    #endif

    if(test_mode)
	fprintf(stderr, "Test mode, formatting job %s for printer %s.\n", QueueFile, printer.Name);

    /*
    ** Become process group leader so we can
    ** kill all of our children if we want to.
    */
    if(setpgid(0, 0) == -1)
    	fatal(EXIT_PRNERR_NORETRY, "pprdrv: setpgid(0, 0) failed, errno=%d (%s)", errno, gu_strerror(errno));

    /* Install handlers for SIGTERM (termination by pprd)
       and for other signals we might receive for some
       weird reason. */
    signal_interupting(SIGTERM, sigterm_handler);;
    signal_interupting(SIGHUP, sigterm_handler);
    signal_interupting(SIGINT, sigterm_handler);
    signal_restarting(SIGCHLD, sigchld_handler);
    signal_interupting(SIGALRM, sigalrm_handler);
    signal_interupting(SIGPIPE, sigpipe_handler);

    /* Read the printer configuration. */
    DODEBUG_MAIN(("main(): pprdrv_read_printer_conf()"));
    pprdrv_read_printer_conf();
    DODEBUG_MAIN(("main(): interface=\"%s\", address=\"%s\", options=\"%s\"", printer.Interface, printer.Address, printer.Options));
    DODEBUG_MAIN(("main(): feedback=%s, jobbreak=%d, codes=%d", printer.Feedback ? "TRUE" : "FALSE", printer.Jobbreak, (int)printer.Codes));

    /* If the outputorder is still unknown, make it forward. */
    if(printer.OutputOrder == 0)
	printer.OutputOrder = 1;

    /*
    ** If we are running in test mode, change some things.
    */
    if(test_mode)
    	{
    	printer.Name = "-";		/* for alert() */
    	printer.charge.per_duplex = 0;
    	printer.charge.per_simplex = 0;
	printer.Feedback = FALSE;
	printer.Jobbreak = JOBBREAK_NONE;
	printer.OutputOrder = 1;
	printer.do_banner = BANNER_FORBIDDEN;
	printer.do_trailer = BANNER_FORBIDDEN;
    	}

    /*
    ** Call the printer status tracking code so that it can load saved status.
    */
    ppop_status_init();

    /*
    ** Open the queue file.  We will keep it open until it is
    ** close automatically when we exit.
    */
    DODEBUG_MAIN(("main(): reading queue file"));
    {
    char fname[MAX_PPR_PATH];
    ppr_fnamef(fname, "%s/%s", QUEUEDIR, QueueFile);
    if((qstream = fopen(fname, "r")) == (FILE*)NULL)
    	fatal(EXIT_JOBERR, "can't open queue file \"%s\", errno=%d (%s)", fname, errno, gu_strerror(errno));
    gu_set_cloexec(fileno(qstream));
    }

    /*
    ** Read miscelaneous data from the queue file
    ** into the structure "job".
    */
    if(read_struct_QFileEntry(qstream, &job))
	fatal(EXIT_JOBERR, "Defective queue file data.");

    /*
    ** We really should make sure this is a queue file format
    ** which we understand.
    */
    if(job.PPRVersion < 1.50)
	{
	give_reason("Obsolete PPR job format");
	fatal(EXIT_JOBERR, "Obsolete PPR job format");
	}

    /*
    ** Parse the queue file name into the structure "job".  Without
    ** regular expressions the parsing is a little tricky, so we
    ** share the function parse_qfname() with pprd.
    */
    {
    char *p = gu_strdup(QueueFile);
    int e = parse_qfname(p, &job.destnode, &job.destname, &job.id, &job.subid, &job.homenode);
    if(e < 0)
    	fatal(EXIT_JOBERR, "parse_qfname() failed, e=%d", e);
    job.destnode = gu_strdup(job.destnode);
    job.destname = gu_strdup(job.destname);
    job.homenode = gu_strdup(job.homenode);
    gu_free(p);
    }

    /* Read and discard the AddOn section. */
    DODEBUG_MAIN(("main(): Discarding Addon section"));
    {
    char *line = NULL;
    int line_max = 256;
    while((line = gu_getline(line, &line_max, qstream)))
	{
	if(strcmp(line, "EndAddon") == 0)
	    {
	    gu_free(line);
	    break;
	    }
	}
    }

    /* Read the lines which list the required media. */
    DODEBUG_MAIN(("main(): calling read_media_lines()"));
    read_media_lines(qstream);

    /*
    ** Read the requirement lines from the queue file and
    ** and determine if we can meet the requirements.
    ** This routine takes the ProofMode into account.
    ** (See RBII p. 664.)
    */
    DODEBUG_MAIN(("main(): calling check_if_capable()"));
    if(check_if_capable(qstream, group_pass))
	{
	DODEBUG_MAIN(("main(): not capable, exiting"));
	return EXIT_INCAPABLE;
	}

    /* Read any PJL header lines in the queue file: */
    {
    char *line = NULL;
    int line_max = 80;
    int n;
    while((line = gu_getline(line, &line_max, qstream)))
	{
	if(strcmp(line, "EndPJL") == 0)
	    {
	    gu_free(line);
	    break;
	    }
	if(gu_sscanf(line, "PJLHint: %d", &n) == 1)
	    {
	    char *p = (char*)gu_alloc(n + 1, sizeof(char));
	    fread(p, sizeof(char), n, qstream);
	    p[n] = '\0';
	    job.PJL = p;	/* <-- possible memory leak !!! */
	    }
	}
    }

    /* Give the N-Up machinery time to ask for its resources. */
    prestart_N_Up_hook();

    /*
    ** Compute the number of physical pages and sheets consumed
    ** by one copy of this job.
    */
    page_computations();

    /* Print banner or trailer page. */
    DODEBUG_MAIN(("main(): calling print_flag_page(%d, 0)", printer.OutputOrder));
    print_flag_page(printer.OutputOrder,0);

    /* Download any patchfile if it is not already downloaded. */
    DODEBUG_MAIN(("main(): patchfile()"));
    patchfile();

    /*
    ** Possibly get the starting page count.
    */
    pagecount_start = pagecount();

    /*
    ** If feedthru mode is turned on, just copy the origional job file
    ** and then skip to the end.
    */
    if(job.opts.hacks & HACK_TRANSPARENT)
	{
	DODEBUG_MAIN(("main(): sending job in transparent mode"));
	transparent_hack_or_passthru_copy(QueueFile, TRUE);
	goto feedthru_skip_point;
	}
    if(job.PassThruPDL)
	{
	DODEBUG_MAIN(("main(): sending PassThruPDL job"));
	transparent_hack_or_passthru_copy(QueueFile, FALSE);
	goto feedthru_skip_point;
	}

    /*
    ** Open the job files.
    */
    DODEBUG_MAIN(("main(): opening job files"));
    {
    char fname[MAX_PPR_PATH];

    ppr_fnamef(fname, "%s/%s-comments", DATADIR, QueueFile);
    if((comments = fopen(fname, "rb")) == (FILE*)NULL)
	fatal(EXIT_JOBERR, "can't open \"%s\", errno=%d (%s)", fname, errno, gu_strerror(errno));
    gu_set_cloexec(fileno(comments));

    ppr_fnamef(fname, "%s/%s-pages", DATADIR, QueueFile);
    if((page_comments = fopen(fname, "rb")) == (FILE*)NULL)
	fatal(EXIT_JOBERR, "can't open \"%s\", errno=%d (%s)", fname, errno, gu_strerror(errno));
    gu_set_cloexec(fileno(page_comments));

    ppr_fnamef(fname, "%s/%s-text", DATADIR, QueueFile);
    if((text = fopen(fname, "rb")) == (FILE*)NULL)
	fatal(EXIT_JOBERR, "can't open \"%s\", errno=%d (%s)", fname, errno, gu_strerror(errno));
    gu_set_cloexec(fileno(text));
    }

    /* Download any persistent fonts or other resources. */
    DODEBUG_MAIN(("main(): persistent_download_now()"));
    persistent_download_now();

    /*
    ** Select a multiple copies method
    */
    select_copies_method();

    /*
    ** Start of the loop we used the print multiple copies by sending
    ** the whole job multiple times in stead of repeating the script section.
    */
    while(copies_doc_countdown--)
	{
	job_start(JOBTYPE_THEJOB);

	/* write the document over the pipe */

	/* copy header comments */
	DODEBUG_MAIN(("main(): calling copy_header()"));
	copy_header();

	/* copy document defaults */
	DODEBUG_MAIN(("main(): calling copy_defaults()"));
	copy_defaults();

	/*
	** If the job contains not marked prolog section,
	** include a fake one at the top.
	*/
	if(!job.attr.prolog)
	    {
	    printer_puts("% No DSC prolog, PPR will add code here ---v\n");

	    /*
	    ** Give the code in pprdrv_patch.c a change to include
	    ** any *JobPatchFile sections from the PPD file.
	    */
	    jobpatchfile();

	    /*
	    ** Include any extra resources such as the N-Up
	    ** dictionary or the TrueType dictionary.
	    */
	    insert_extra_prolog_resources();

	    printer_puts("% ^--- End of non-DSC prolog\n");
	    }

	/*
	** If this document does not have a marked docsetup section,
	** include a fake one at the top.
	*/
	if(!job.attr.docsetup)			/* if no conforming doc setup section, */
	    {					/* put non-conforming one at job start */
	    printer_puts("% No DSC document setup, PPR will add code here ---v\n");
	    set_jobname();
	    insert_noinclude_fonts();
	    if(media_count==1 && job.opts.binselect)
		select_medium(media_xlate[0].pprname);
	    insert_features(qstream, 1);	/* insert most ppr -F switch things */
	    invoke_N_Up();
	    insert_features(qstream, 2);	/* insert ppr -F *Duplex switch thing */
	    if(job.opts.copies != -1)		/* don't worry about copies_auto==FALSE */
		printer_printf("/showpage { /#copies %d def showpage } bind def %%PPR\n", job.opts.copies);
	    custom_hook(CUSTOM_HOOK_DOCSETUP, 0);
	    printer_puts("% ^--- end of non-DSC document setup\n");
	    }

	/*
	** Copy the main prolog, document setup, and script sections:
	*/
	DODEBUG_MAIN(("main(): copying various code sections"));
	if(copy_prolog())		/* Copy prolog, and if it did not end */
	    {
	    if(copy_setup())		/* with "%%Trailer", copy Document Setup */
		{
		if(! copy_nonpages() )	/* if it didn't end, copy non-page text */
		    {
		    custom_hook(CUSTOM_HOOK_PAGEZERO, 0);
		    copy_pages();	/* and then if still no end, copy pages. */
		    }
		}
	    }

	/* copy the trailer */
	DODEBUG_MAIN(("main(): calling copy_trailer()"));
	copy_trailer();

	/*
	** If we are printing multiple copies by sending the
	** whole document again and again, then send end of
	** file to the printer and rewind those files and
	** go back and do it again.  This clause is not
	** ``true'' on the last copy.
	*/
	if(copies_doc_countdown)
	    {
	    rewind(comments);
	    rewind(page_comments);
	    rewind(text);
	    }

	job_end();
	} /* this bracket goes with the start of the copies loop */

    /* close the files */
    fclose(qstream);            /* close the queue file */
    fclose(comments);           /* close the comments file */
    fclose(page_comments);      /* close the page comments and index file */
    fclose(text);               /* close the body text file */

    /* Point to skip to after doing a raw copy: */
    feedthru_skip_point:

    /* Try to figure out how many pages were printed. */
    pagecount_change = -1;
    if(pagecount_start != -1)
    	{
    	int pagecount_end;
    	if((pagecount_end = pagecount()) != -1)
	    pagecount_change = (pagecount_end - pagecount_start);
	}

    /*
    ** Print a header or trailer page if proper to do so.
    ** We must close the job log so we can print it.
    */
    DODEBUG_MAIN(("main(): calling print_flag_page()"));
    print_flag_page(printer.OutputOrder*-1, 1);

    /*
    ** Close the pipes to the interface and wait
    ** for it to exit.
    */
    DODEBUG_MAIN(("main(): calling job_nomore()"));
    result = job_nomore();
    DODEBUG_MAIN(("main(): interface terminated with code %d", result));

    /*
    ** If we received a message about a PostScript error
    ** then the result is considered a job error no matter
    ** what happened.  This is because some printers will
    ** cut off the connexion after a PostScript error.
    **
    ** See the similiar code toward the end of reapchild().
    */
    if(feedback_posterror())
	result = EXIT_JOBERR;

    /*
    ** If we charge for the use of this printer and the job printed normally,
    ** then charge now.  (But don't bother posting charges of zero.)
    */
    if(result == EXIT_PRINTED && (printer.charge.per_duplex > 0 || printer.charge.per_simplex > 0))
	{
	/*
	** Of course, we can't charge nobody.  Not that we would
	** expect such a job to get this far, though it could be that
	** the printer was protected after this job entered the queue.
	*/
	if(job.charge_to == (char*)NULL)
	    {
	    error("main(): can't charge because \"%%%%For:\" line is blank!");
	    }
	else
	    {
	    struct COMPUTED_CHARGE charge;

	    compute_charge(&charge,
	    	printer.charge.per_duplex, printer.charge.per_simplex,
	    	pagemask_count(&job), job.N_Up.N, job.attr.pagefactor,
	    	job.N_Up.sigsheets, job.N_Up.sigpart, job.opts.copies);

	    if(db_transaction(job.charge_to, charge.total, TRANSACTION_CHARGE) != USER_OK)
		error("main(): failed to charge user using db_transaction()");
	    }
	}

    /* If printing was sucessful and pprdrv is not running with
       the --test switch (in which case the printer name would
       be "-"), then add an entry to the printlog. */
    if(result == EXIT_PRINTED && strcmp(printer.Name, "-"))
	printer_use_log(&start_time, pagecount_start, pagecount_change);

    /*
    ** Tell the commentators why we are exiting.
    **
    ** Note that exiting anywhere else will call pprdrv_fault_debug.c:exit_hook()
    ** which will take care of calling commentator_exit_hook().
    */
    if(feedback_posterror())
	commentary_exit_hook(result, "postscript error");
    else
	commentary_exit_hook(result, (char*)NULL);

    /*
    ** Allow the "ppop status" system to infer anything it wants to from
    ** our exit code.
    **
    ** Note that exiting anywhere else will call pprdrv_fault_debug.c:exit_hook()
    ** which will take care of calling ppop_status_exit_hook().
    **
    ** Also note that this call may launch commentators, so it must go before
    ** the call to commentator_wait().
    */
    ppop_status_exit_hook(result);

    /*
    ** Give commentator processes 60 more seconds to do there jobs
    ** and then kill any that are still living.
    */
    commentator_wait();

    /*
    ** Let the "ppop status" system clear the "waiting for commentators" state.
    */
    ppop_status_shutdown();

    /*
    ** Exit with a code that probably indicates sucess or a job error.
    */
    DODEBUG_MAIN(("main(): exiting with code %d", result));
    return result;
    } /* end of main() */

/* end of file */
