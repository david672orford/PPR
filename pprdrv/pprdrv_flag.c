/*
** mouse:~ppr/src/pprdrv/pprdrv_flag.c
** Copyright 1995--2001, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Last modified 20 July 2001.
*/

/*
** This module contains header and trailer page generation code.
*/

#include "before_system.h"
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "global_structs.h"
#include "pprdrv.h"
#include "interface.h"
#include "userdb.h"
#include "version.h"


/*
** Global variables for this module.
** For the sake of neatness these are gathered into a structure.
*/
static struct
    {
    char *medianame;            /* name of selected media */
    double ph;                  /* page height in 72ths of an inch */
    double pw;                  /* page width in 72ths of an inch */
    double weight;
    char *colour;
    char *type;
    int mar;                    /* margin */
    int bor;                    /* border */
    int large_size;             /* point size of large type */
    int small_size;             /* point size of small type */
    int log_size;               /* point size of log file type */
    } flag;

/*
** If job.For consists of two words, reverse them and
** seperate them with a comma.
**
** In any event, return a pointer to what we should use
** on the banner page.
*/
static const char *reversed_for(void)
    {
    char *spc_ptr;
    static const char *result = (char*)NULL;

    /* This should never happen: */
    if(!job.For)
    	return "???";

    /* If we have already figured it out, return the old result. */
    if(result)
    	return result;

    /* If search for a space fails, stop now. */
    if((spc_ptr = strchr(job.For, ' ')) == (char*)NULL)
	result = job.For;

    /* If search for another space succeeds, stop now. */
    else if(strchr(spc_ptr+1, ' '))
	result = job.For;

    else
    	{
	/* Get a block to hold all the characters, a NULL and an added comma. */
	char *ptr = (char*)gu_alloc(strlen(job.For)+2, sizeof(char));

	strcpy(ptr, spc_ptr+1);		/* take the part after the space */
	strcat(ptr, ", ");		/* add a comma and a space */
	*spc_ptr = '\0';		/* replace origional space with null */
	strcat(ptr, job.For);		/* add null terminated 1st part */
	*spc_ptr = ' ';			/* put the space back */

	result = ptr;
	}

    return result;
    } /* end of reverse_for() */

/*
** Print a formatted line to the interface.  This is used
** when printing the banner page text.
*/
static void pslinef(const char *string, ... )
    {
    va_list va;

    printer_putc('(');			/* Start PostScript string */

    va_start(va, string);

    while(*string)
	{
	if(*string == '%')
	    {
	    string++;			/* discard the "%%" */
	    switch(*(string++))
		{
		case 's':		/* a string */
		    printer_puts_escaped(va_arg(va, char *));
		    break;
		case 'd':		/* a decimal value */
		    printer_printf("%d", va_arg(va, int));	/* !!! */
		    break;
		default:
		    fatal(EXIT_PRNERR_NORETRY, "pslinef(): illegal format spec: %s", string-2);
		    break;
		}
	    }
	else
	    {
	    printer_putc_escaped(*(string++));
	    }
	}

    va_end(va);

    printer_puts(")p\n");	/* end PostScript string and add print line command */
    } /* end of pslinef() */
/*
** Tabulate the votes as to whether we should have a banner or trailer page.
** Return TRUE if we should, FALSE if we should not.
**
** Printer will vote BANNER_REQUIRED, BANNER_ENCOURAGED, BANNER_DISCOURAGED,
** BANNER_FORBIDDEN.  Job will vote BANNER_YESPLEASE, BANNER_DONTCARE,
** or BANNER_NOTHANKYOU.
**
** New feature: if there is a log and this flag is being printed last, and
** the banner or trailer, as applicable, is not prohibited, print the flag
** even if the user or printer configuration have not requested it.
*/
static int flag_page_vote(int printer_vote, int job_vote, int position)
    {
    DODEBUG_FLAGS(("flag_page_vote(%d,%d)",printer_vote,job_vote));

    switch(printer_vote)
	{
	case BANNER_FORBIDDEN:			/* if printer doesn't allow, */
	    return FALSE;			/* never do it */
	case BANNER_DISCOURAGED:		/* if printer discourages */
	    if(job_vote==BANNER_YESPLEASE)	/* then, only if asked for */
		return TRUE;
	    else
		return FALSE;
	case BANNER_ENCOURAGED:
	    if(job_vote==BANNER_NOTHANKYOU)
		return FALSE;
	    else
		return TRUE;
	case BANNER_REQUIRED:
	    return TRUE;
	default:                                /* this is an error */
	    fatal(EXIT_PRNERR_NORETRY, "pprdrv: flag_page_vote(): no case for printer_vote=%d",printer_vote);
	}
    } /* end of flag_page_vote() */

/*
** This routine decided which medium will be used for the flag pages.
**
** It fills in the structure "flag" with information about the appropriate
** medium.  If the medium is known to be mounted in a specific bin, the
** actual bin selection is done later using select_media().
**
** The return values have the following meaning:
**
**	-1	Flag pages printing is prohibited on all mounted media.
**		No flag pages will be printed if this value is returned.
**		Nor will the "flag" structure be filled in.
**
**	0	Not defined for this printer.  The "flag" structure will
**		be filled in with a description of the default medium
**		as described in /etc/ppr/ppr.conf.
**
**	1	At least one bin contains a suitable medium.  The "flag"
**		structure now contains a description of it.  Please
**		call select_media() to select it.
*/
static int select_flag_medium(void)
    {
    FUNCTION4DEBUG("select_flag_medium")
    static int select_flag_medium_count = 0;	/* tells if we were called yet */
    static int rval = -1;			/* error indicator to return */
    FILE *mediafile;				/* list of all media */
    struct Media media[MAX_BINS];		/* the medium in each bin */
    struct Media t;				/* temporary, for reading */
    static char static_medium_name[MAX_MEDIANAME+1];
    static char static_medium_colour[MAX_COLOURNAME+1];
    static char static_medium_type[MAX_TYPENAME+1];
    int x, y;					/* for looping */
    int bincount;				/* # of entries in mounted file */
    int found_count = 0;			/* # found in media file */

    DODEBUG_FLAGS(("%s()", function));

    /*
    ** If this routine was called before, just return the same value as
    ** we returned last time.  The structure "flag" is still valid.
    */
    if(select_flag_medium_count++ > 0)
	{
	DODEBUG_FLAGS(("%s(): proper medium already determined", function));
	return rval;        /* (on 2nd and subsequent calls, do nothing) */
	}

    /* Load the list if media mounted on this printer's bins
       into the global array media[]. */
    if((bincount = load_mountedlist()) > 0)   /* If mounted list found and not zero bins, */
	{
	for(x=0; x < MAX_BINS; x++)
	    media[x].flag_suitability = 0;

	/* Open the PPR media description database.  This database maps the
	   media names used with "ppop media mount" to media descriptions.
	   These descriptions include a ranking of the medium's suitability
	   for use as a banner page.  1 means totally unsuitable (probably
	   because it is too expensive), 10 means that it is an excelent thing
	   to print banner pages on. */
	if((mediafile = fopen(MEDIAFILE, "rb")) == (FILE*)NULL)
	    fatal(EXIT_PRNERR_NORETRY, _("Can't open \"%s\", errno=%d (%s)"), MEDIAFILE, errno, gu_strerror(errno));

	/* Find each medium in the media[] array in the database and fill
	   in its detailed information, including its banner page suitability. */
	while((fread(&t,sizeof(struct Media),1,mediafile) == 1) && (found_count < bincount) )
	    {
	    for(x=0; x < bincount; x++)
		{
		if(strncmp(t.medianame, mounted[x].media, MAX_MEDIANAME) == 0)
		    {
		    memcpy(&media[x], &t, sizeof(struct Media));
		    found_count++;
		    DODEBUG_FLAGS(("one match!"));
		    }
		}
	    }

	fclose(mediafile);

	/*
	** Examine the medium which we have just determined is mounted
	** on each bin in order to find the most suitable.
	**
	** The variable y is used to keep track of the highest suitability
	** rating found so far.  It is y initialy set to 1 because a medium
	** with a suitability rating of one is not usable for banner pages.
	*/
	for(x=0,y=1; x < bincount; x++)		/* find the most suitable of */
	    {					/* those suitable */
	    if(media[x].flag_suitability > y)
		{
		y = media[x].flag_suitability;	/* this is best found yet */

		flag.medianame = static_medium_name;	/* we use this */
		padded_to_ASCIIZ(static_medium_name, media[x].medianame, MAX_MEDIANAME);

		flag.pw = media[x].width;		/* we use this */
		flag.ph = media[x].height;		/* we use this */
		flag.weight = media[x].weight;		/* we don't use this */

		flag.colour = static_medium_colour;	/* we don't use this */
		padded_to_ASCIIZ(static_medium_colour, media[x].colour, MAX_COLOURNAME);

		flag.type = static_medium_type;		/* we don't use this */
		padded_to_ASCIIZ(static_medium_type, media[x].type, MAX_TYPENAME);

		rval = 1;			/* we did it! */
		}
	    }

	} /* end of if sucessfully opened mounted file */
    else
	{
        const char file[] = PPR_CONF;
        const char section[] = "internationalization";
        const char key[] = "defaultmedium";
        const char *error_message;

	DODEBUG_FLAGS(("%s(): no bins defined, bincount=%d"));

        error_message = gu_ini_scan_list(file, section, key,
            GU_INI_TYPE_NONEMPTY_STRING, &flag.medianame,
            GU_INI_TYPE_POSITIVE_DOUBLE, &flag.pw,
            GU_INI_TYPE_POSITIVE_DOUBLE, &flag.ph,
            GU_INI_TYPE_NONNEG_DOUBLE, &flag.weight,
            GU_INI_TYPE_STRING, &flag.colour,
            GU_INI_TYPE_STRING, &flag.type,
            GU_INI_TYPE_END);

        if(error_message)
            {
            error(_("%s: %s\n"
                            "\twhile attempting to read \"%s\"\n"
                            "\t\t[%s]\n"
                            "\t\t%s =\n"),
		    "select_flag_medium()",
                    gettext(error_message), file, section, key);

            flag.medianame = "Letter";
            flag.pw = 612;		/* 8.5 inches */
            flag.ph = 792;		/* 11 inches */
            flag.weight = 0;
            flag.colour = "white";
            flag.type = "";		/* blank paper */
            }

	rval = 0;			/* arrange to return 0 */
	}

    /* compute some things based on the selected form size */
    if(rval != -1)
	{
        flag.mar = (int)( ( flag.ph > flag.pw ? flag.pw : flag.ph ) / 15 );
        flag.bor = (int)(flag.mar * 0.70);
        flag.large_size = (int)(flag.ph / 20);
        flag.small_size = (flag.large_size / 3);
        flag.log_size = (int)((flag.pw - flag.mar) / 80 / 0.6);
	}

    return rval;
    } /* end of select_flag_medium() */

/*
** This is called to insert PostScript resources we need for the banner page.
**
** The normal insert_resource() routine can't be used because it can only
** insert resources which are required by the main document.
*/
static void mgu_ini_insert_resource(const char restype[], const char resname[], double version, int revision)
    {
    const char function[] = "mgu_ini_insert_resource";
    FILE *f;
    char fname[MAX_PPR_PATH];
    int c;

    if(version == 0.0)
	ppr_fnamef(fname, "%s/%s/%s", STATIC_CACHEDIR, restype, resname);
    else
	ppr_fnamef(fname, "%s/%s/%s-%s-%d", STATIC_CACHEDIR, restype, resname, gu_dtostr(version), revision);

    if((f = fopen(fname, "r")) == NULL)
        fatal(EXIT_PRNERR_NORETRY, "%s(): can't open \"%s\" for read, errno=%d (%s)", function, fname, errno, gu_strerror(errno));

    printer_printf("%%%%BeginResource: %s %s", restype, resname);
    if(version != 0.0)
        printer_printf(" %s %d", gu_dtostr(version), revision);
    printer_putc('\n');

    while((c = fgetc(f)) != EOF)
        printer_putc(c);

    printer_puts("%%EndResource\n");

    fclose(f);
    } /* end of mgu_ini_insert_resource() */

/*
** This is the routine which actually emmits the PostScript for the
** standard flag page.
**
** flag_type 1		header page
** flag_type -1		trailer page
** position 0		before job
*/
static void print_flag_page_standard(int flag_type, int position, int mediumfound)
    {
    FILE *logfile;

    /* Get interface and printer ready to receive the flag page. */
    job_start(JOBTYPE_FLAG);

    /* Send the PostScript DSC comments section to the printer */
    printer_puts("%!PS-Adobe-3.0\n");
    printer_puts("%%Creator: PPR "VERSION"\n");

    if(flag_type<0)
	printer_puts("%%Title: Trailer Page\n");
    else
	printer_puts("%%Title: Banner Page\n");

    printer_puts("%%DocumentNeededResources: font Courier\n");

    printer_puts("%%Pages: 1\n");

    printer_printf("%%%%DocumentMedia: %s %f %f %f (%s) (%s)\n",
		flag.medianame, flag.pw, flag.ph, flag.weight,
		flag.colour, flag.type);

    /* Level 2 printers have ISOLatin1Encoding already defined,
       Level 1 printers don't. */
    printer_puts("%%DocumentSuppliedResources: procset TrinColl-PPR-ReEncode 1.1 0\n");
    if(Features.LanguageLevel == 1)
	{
	printer_puts("%%+ encoding ISOLatin1Encoding\n");
	printer_puts("%%LanguageLevel: 1\n");
	}
    else
    	{
    	printer_puts("%%LanguageLevel: 2\n");
    	}

    printer_puts("%%EndComments\n\n");

    /* send the prolog to the printer */
    printer_puts("%%BeginProlog\n");

    /* newline routine */
    printer_printf("/nl{/y y ys sub def}bind def\n");

    /* printline routine */
    printer_printf("/p{/y y ys 0.7 mul sub def %d y moveto show "
	    "/y y ys 0.3 mul sub def}bind def\n",flag.mar);

    /* ISOLatin1Encoding is needed for level 1 printers. */
    if(Features.LanguageLevel < 2)
	mgu_ini_insert_resource("encoding", "ISOLatin1Encoding", 0.0, 0);

    /* Procedure for re-encoding in ISO Latin 1: */
    mgu_ini_insert_resource("procset", "TrinColl-PPR-ReEncode", 1.1, 0);

    printer_puts("%%EndProlog\n\n");

    /* send the document setup section */
    printer_puts("%%BeginSetup\n");

    if(mediumfound == 1)	/* If at least one bin had it, */
	{
	int x;

	/*
	** Select the correct bin.  We will ignore the return
	** value because it should always be TRUE.  In fact, I
	** find it difficult to imagine how it could be FALSE.
	*/
	select_media(flag.medianame);

	/*
	** Look thru the paper sizes, and if we find one
	** that matches, use it.
	*/
	for(x=0; x < num_papersizes; x++)
	    {
	    if(papersize[x].width == flag.pw && papersize[x].height == flag.ph)
	    	{
		begin_stopped();
		include_feature("*PageRegion", papersize[x].name);
		end_stopped("*PageRegion", papersize[x].name);
		break;
	    	}
	    }
	} /* end of if(mediumfound == 1) */

    /*
    ** Theoretically, we insert the font here.  The origional code was:
    ** _include_resource("font","Courier",0.0,0);
    ** however, it won't work anymore since _include_resource() can now
    ** only include resources which are pre-declared.  We really want
    ** the default font if Courier is not available, so we will say:
    */
    printer_puts("%%IncludeResource: font Courier\n");
    printer_puts("/Courier /Courier /ISOLatin1Encoding ReEncode\n");

    /*
    ** Scale the font to two sizes.  Large is for the user name,
    ** small is for the other messages.
    */
    printer_printf("/LargeFont /Courier findfont %d scalefont def\n", flag.large_size);
    printer_printf("/SmallFont /Courier findfont %d scalefont def\n", flag.small_size);

    /* Define routines to select the two font sizes. */
    printer_printf("/large_type{LargeFont setfont /ys %d def}bind def\n",(int)(flag.large_size*1.3));
    printer_printf("/small_type{SmallFont setfont /ys %d def}bind def\n",(int)(flag.small_size*1.3));

    printer_puts("%%EndSetup\n\n");

    /* send the first part of the page */
    printer_puts("%%Page: one 1\n");
    printer_puts("save\n");
    printer_printf("/y %d def\n", (int)(flag.ph-flag.mar));

    /* Draw a box around the page. */
    printer_printf("newpath %d dup moveto 0 %d rlineto %d 0 rlineto 0 -%d rlineto\n",
	(int) flag.bor,
	(int) (flag.ph-(flag.bor*2)),
	(int) (flag.pw-(flag.bor*2)),
	(int) (flag.ph-(flag.bor*2)) );
    printer_puts("closepath stroke\n");

    /* print the text of the page */
    if(flag_type < 0)		/* trailer page */
	{
	printer_puts("large_type ");
	pslinef("%s", reversed_for());

	printer_puts("small_type\n");

	pslinef(_("End of job %s:%s-%d"), job.destnode, job.destname, job.id);

	if(job.Title)
	    pslinef(_("Title: %s"), job.Title);
	}
    else			/* header page */
	{
	printer_puts("large_type ");
	pslinef(reversed_for());

	printer_puts("small_type\n");

	pslinef(_("Job ID: %s:%s-%d"), job.destnode, job.destname, job.id);

	if(job.Routing)
	    pslinef(_("Routing: %s"), job.Routing);

	if(job.Creator)
	    pslinef(_("Creator: %s"), job.Creator);

	if(job.Title)
	    pslinef(_("Title: %s"), job.Title);

	/* print the current time */
	{
	time_t timenow;
	struct tm *timeptr;
	char temp[40];

	char *format = gu_ini_query(PPR_CONF, "internationalization", "flagdateformat", 0, "%d-%b-%Y, %I:%M%p");
	time(&timenow);
	timeptr = localtime(&timenow);
	strftime(temp, sizeof(temp), format, timeptr);
	pslinef(_("Print Date: %s"), temp);
	gu_free(format);
	}

	/* If # of pages known, tell. */
	if(job.attr.pages >= 0)
	    {
	    pslinef(_("Pages: %d"), job.attr.pages);	/* number of PostScript page descriptions */
	    pslinef(_("Sheets: %d"), sheetcount);	/* sheets of paper */
	    }

	/* If multiple copies, tell how many. */
	if(job.opts.copies > 1)
	    {
	    pslinef(_("Copies: %d"), job.opts.copies);
	    }

	/*
	** If we are charging for printing, say so now.
	** Notice that if the destination was not "protected"
	** at the time this job entered the queue, then
	** job.charge_to will be NULL and we won't be able to
	** charge the user for this job.
	**
	** This section is going to be tough to internationalize.
	*/
	if(job.charge_to && (printer.charge.per_duplex > 0 || printer.charge.per_simplex > 0))
	    {
	    struct userdb db;

	    if(db_auth(&db, job.charge_to) == USER_ISNT)
		{
		error("User \"%s\" is inexplicably absent from the charge account database.\n", job.charge_to);
		pslinef(_("User \"%s\" is inexplicably absent from the charge account database."), job.charge_to);
		}
	    else
	    	{
		struct COMPUTED_CHARGE charge;

		pslinef(_("Balance before printing this job: %s"), money(db.balance));

		compute_charge(&charge, printer.charge.per_duplex,
			printer.charge.per_simplex,
			job.attr.pages,
			job.N_Up.N,
			job.attr.pagefactor,
			job.N_Up.sigsheets,
			job.N_Up.sigpart,
			job.opts.copies);

		printer_puts("(Charge for this job: ");

		if(charge.duplex_sheets > 0)	/* if there were duplex sheets, */
		    {
		    printer_printf("%d duplex sheet%s @ ", charge.duplex_sheets, charge.duplex_sheets > 1 ? "s" : "");
		    printer_puts_escaped(money(charge.per_duplex));
		    }

		if(charge.simplex_sheets > 0)	/* if there were simplex sheets, */
		    {
		    if(charge.duplex_sheets)
		    	printer_puts(")p\n(      + ");
		    printer_printf("%d simplex sheet%s @ ", charge.simplex_sheets, charge.simplex_sheets > 1 ? "s" : "");
		    printer_puts_escaped(money(charge.per_simplex));
		    }

		printer_puts(" = ");
		printer_puts_escaped(money(charge.total));
		printer_puts(")p\n");
		}
	    }
	} /* end of if header page */

    /* print the log file contents */
    {
    char fname[MAX_PPR_PATH];
    char buffer[82];			/* buffer for reading log file */
    feedback_close_log();	/* flush out messages from printer */
    ppr_fnamef(fname, "%s/%s-log", DATADIR, QueueFile);
    if((logfile = fopen(fname, "r")) != (FILE*)NULL)
	{
	#ifdef DEBUG_FLAGS
	debug("print_flag_page(): printing log contents");
	#endif

	printer_printf("/Courier findfont %d scalefont setfont\n", flag.log_size);
	printer_printf("/ys %d def\n", (int)(flag.log_size*1.2));
	printer_putline("nl");

	while(fgets(buffer,sizeof(buffer),logfile))
	    {
	    /* Skip job redirection description lines. */
#if 0
	    if(buffer[0] == '+') continue;
#endif
	    buffer[strcspn(buffer, "\n")] = '\0';	/* remove trailing space */
	    printer_puts("("); printer_puts_escaped(buffer); printer_puts(")p\n");
	    }

	fclose(logfile);

	if(position == 0)   	/* if this page was printed before the job, */
	    unlink(fname);  	/* then unlink the log file */
	} /* logfile exists */
    }

    /* close the page */
    printer_puts("restore showpage\n");

    /* close the postscript */
    printer_puts("\n%%Trailer\n");
    printer_puts("%%EOF\n");

    /* Tell printer setup and interface routines
       that the job is done. */
    job_end();
    } /* end of print_flag_page_standard() */

/*
** This is the only externally called routine in this module.
**
** If proper, print a banner or trailer page.
**
** Flag type of 1 is header, -1 is trailer.
** position is 0 if at start of job, 1 if at end of job.
*/
void print_flag_page(int flag_type, int position)
    {
    DODEBUG_FLAGS(("print_flag_page(flag_type=%d, position=%d)", flag_type, position));

    /* decide if we should print a flag page */
    if(flag_type < 0)           /* trailer page */
	{
	if( !flag_page_vote(printer.do_trailer, job.do_trailer, position) )
	    return;
	}
    else                        /* banner page */
	{
	if( !flag_page_vote(printer.do_banner, job.do_banner, position) )
	    return;
	}

    /* Give a custom flag page program a chance to run.  If it declines,
       print our own flag page. */
    if(!custom_hook((flag_type > 0) ? CUSTOM_HOOK_BANNER : CUSTOM_HOOK_TRAILER, position))
	{
        /* Look for a suitable medium.  If select_flag_medium() indicates
           that bins exist but none of them have suitable media mounted,
           abort flag printing now. */
        int mediumfound;
        if((mediumfound = select_flag_medium()) == -1)
            {
            DODEBUG_FLAGS(("all mounted media are unsuitable for flag pages"));
            return;
            }
	/* Print a flag page on the selected medium. */
        print_flag_page_standard(flag_type, position, mediumfound);
        }
    } /* end of print_flag_page() */

/* end of file */

