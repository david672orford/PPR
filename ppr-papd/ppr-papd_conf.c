/*
** mouse:~ppr/src/ppr-papd/ppr-papd_conf.c
** Copyright 1995--2002, Trinity College Computing Center.
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
** Last modified 19 November 2002.
*/

#include "before_system.h"
#include <stdlib.h>
#include <ctype.h>
#include <sys/stat.h>
#include <string.h>
#include <memory.h>
#include <errno.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "ppr-papd.h"

int yylex(void);
extern FILE *yyin;
struct ADV *printer;			/* for communicating with yylex() */

char *ppd_nest_fname[MAX_PPD_NEST];	/* names of all nested PPD files */
int ppd_nest_level;			/* number of PPD files now open */

/*======================================================================
** Font id routines
======================================================================*/
static char **fonts=(char **)NULL;	/* pointer to an array of character pointers */
static int fonts_count=0;		/* number of fonts in the array */
static int fonts_space=0;		/* number of array slots allocated */

/*
** Given a font name, return an id number
** which will represent it in future.
*/
static SHORT_INT make_font_id(const char fontname[])
    {
    int x;

    for(x=0; x < fonts_count; x++)
	{
	if(strcmp(fonts[x], fontname) == 0)
    	    return x;
	}

    if(fonts_count == fonts_space)	/* if more space needed */
	{
	fonts_space += 50;		/* make space for 50 more */
	fonts = (char**)gu_realloc(fonts, fonts_space, sizeof(char*) );
	}

    fonts[fonts_count++] = gu_strdup(fontname);

    return x;
    } /* end of make_font_id() */

/*
** If the font name supplied is known, return the font id
** otherwise, return -1.
*/
SHORT_INT get_font_id(const char fontname[])
    {
    int x;

    for(x=0; x < fonts_count; x++)
    	{
    	if(strcmp(fonts[x], fontname) == 0)
    	    return x;
    	}

    return -1;
    } /* end of get_font_id() */

/*
** Given a font id, return the font name.
** If the font id is not valid, return a NULL string pointer.
*/
const char *get_font_name(SHORT_INT fontid)
    {
    if(fontid >= fonts_count || fontid < 0)
    	return (const char*)NULL;

    return fonts[fontid];
    } /* end of get_font_name() */

/*====================================================================
** Routines for identifying and reading the PPD file
====================================================================*/

/*
** Given a printer name, return the PPD file name for the printer.
** The new value is in allocated memory.
**
** The variable entry_index is the adv[] array position at which
** we should store PPD option settings we happen to read.  If it
** is -1, will will ignore them.
*/
static char *get_printer_ppd(const char name[], int entry_index)
    {
    const char function[] = "get_printer_ppd";
    FILE *pf;
    char *line = NULL;
    int line_len = 128;
    char *rval = NULL;
    char *p;

    DODEBUG_STARTUP(("%s(name=\"%s\", entry_index=%d)", function, name, entry_index));

    /* Open the printer's configuration file. */
    {
    char fname[MAX_PPR_PATH];
    ppr_fnamef(fname, "%s/%s", PRCONF, name);
    if((pf = fopen(fname, "r")) == (FILE*)NULL)
	fatal(0, "%s(): fopen(\"%s\",\"r\") failed, errno=%d (%s)", function, fname, errno, gu_strerror(errno));
    }

    /* Read the printer's configuration file. */
    while((line = gu_getline(line, &line_len, pf)))
	{
	if(gu_sscanf(line, "PPDFile: %Z", &p) == 1)
	    {
	    if(rval) gu_free(rval);
	    rval = p;
	    }

	else if(entry_index != -1 && strncmp(line, "PPDOpt:", 7) == 0)
	    {
	    int len;
	    char *name, *value;

	    name = &line[7+strspn(&line[7]," \t")];	/* skip to start of first word */
	    len = strcspn(name," \t");			/* determine length of first word */
	    name[len] = '\0';				/* terminate  first word */

	    value = name + len + 1;			/* move past first word and terminator */
	    value += strspn(value, "\t ");		/* move past spaces */
	    len = strcspn(value, " \t\r\n");		/* determine length of second word */
	    value[len] = '\0';				/* terminate second word */

	    if(strcmp(name, "*InstalledMemory") == 0)
	    	{
		adv[entry_index].InstalledMemory = gu_strdup(value);
	    	}
	    else
	    	{
		struct OPTION *opt = (struct OPTION*)gu_alloc(1, sizeof(struct OPTION));
		opt->name = gu_strdup(name);
		opt->value = gu_strdup(value);
	        opt->next = adv[entry_index].options;	/* insert new option in chain */
		adv[entry_index].options = opt;
		}
	    }

	else if(entry_index != -1 && strncmp(line, "Codes:", 6) == 0)
	    {
	    int value = atoi( &line[6+strspn(&line[6], " \t")] );

	    if(value == CODES_Binary || value == CODES_TBCP)
		adv[entry_index].BinaryOK = TRUE;
	    }

	}

    fclose(pf);

    if(!rval)
	fatal(0, _("Printer \"%s\" has no \"PPDFile\" line in its config file"), name);

    return rval;			/* return the result */
    } /* end of get_printer_ppd() */

/*
** Return the PPD file name for a group.  If its printers do not all have
** the same one, return a NULL string pointer.
**
** This function returns the string in a newly allocated heap block.
*/
static const char *get_group_ppd(const char grname[])
    {
    FILE *gf;                   /* group file */
    char *line = NULL;
    int line_len = 128;
    char *ptr;
    int len;
    char *ppdname = (char*)NULL;
    struct stat statbuf;
    int grline = 0;

    DODEBUG_STARTUP(("get_group_ppd(grname=\"%s\")", grname));

    /* open the group configuration file */
    {
    char fname[MAX_PPR_PATH];
    ppr_fnamef(fname, "%s/%s", GRCONF, grname);
    if((gf = fopen(fname, "r")) == (FILE*)NULL)
	fatal(0, "get_group_ppd(): open failed");
    }

    /* read it line by line */
    while((line = gu_getline(line, &line_len, gf)))
	{
	grline++;

	if(strncmp(line, "Printer:", 8) == 0)
	    {
	    ptr = &line[8+strspn(&line[8], " \t")];
	    len = strlen(ptr);			/* remove */
	    while( len && isspace(ptr[--len]) )	/* trailing spaces */
		ptr[len] = '\0';		/* from printer name */

	    {
	    char fname[MAX_PPR_PATH];
	    ppr_fnamef(fname, "%s/%s", PRCONF, ptr); /* check if printer exists */
	    if(stat(fname, &statbuf) < 0)
		fatal(0, _("group \"%s\" member \"%s\" does not exist (%s line %d)"), grname, ptr, fname, grline);
	    }

	    {
	    char *p = get_printer_ppd(ptr, -1);
	    if(ppdname == (char*)NULL)		/* if none found yet, */
		{				/* then use this one */
		ppdname = p;
		}
	    else
		{
		if(strcmp(ppdname, p))
		    {                           /* if not same as last, */
		    gu_free(p);
		    gu_free(ppdname);
		    gu_free(line);
		    fclose(gf);                 /* then close group file */
		    return (const char*)NULL;	/* and tell caller we failed */
		    }
		}
	    }
	    }
	}

    fclose(gf);			/* close group configuration file */
    return ppdname;		/* return the name we settled on */
    } /* end of get_group_ppd() */

/*
** Read a PPD file for the benefit of a specific listen name.
** This function is passed a pointer to the ADV structure for the
** printer, the name of the PPD file, and the current line of the
** PAPSRV configuration file.
*/
static int printer_fontspace;
static void read_ppd(struct ADV *adv, const char ppdname[])
    {
    char *fname;

    DODEBUG_STARTUP(("read_ppd(adv=?, ppdname=\"%s\")", ppdname));

    /* save PPD file name */
    adv->PPDfile = ppdname;

    if(ppdname[0] == '/')
	{
	fname = gu_strdup(ppdname);	/* we free it later */
	}
    else
	{
	size_t space_needed = (sizeof(PPDDIR) + 1 + strlen(ppdname) + 1);
	fname = (char*)gu_alloc(space_needed, sizeof(char));
	snprintf(fname, space_needed, "%s/%s", PPDDIR, ppdname);
	}

    if((yyin = fopen(fname, "r")) == (FILE *)NULL)
	fatal(1, _("Can't open \"%s\", errno=%d (%s)"), fname, errno, gu_strerror(errno));

    printer = adv;		/* set global for benefit of yylex() */
    printer_fontspace = 0;

    ppd_nest_level = 0;
    ppd_nest_fname[ppd_nest_level] = fname;

    yylex();

    fclose(yyin);
    gu_free(fname);

    DODEBUG_STARTUP(("read_ppd(): done"));
    } /* end of read_ppd() */

/*
** This helper function is called by the lexer that reads the PPD file.
*/
void add_font(char *fontname)
    {
    DODEBUG_STARTUP(("add_font(fontname=\"%s\")", fontname));

    if( printer->fontcount == printer_fontspace )	/* if more space needed */
    	{
    	printer_fontspace += 50;
    	printer->fontlist = (SHORT_INT*)gu_realloc((void*)printer->fontlist, printer_fontspace, sizeof(SHORT_INT) );
    	}

    printer->fontlist[printer->fontcount++] = make_font_id(fontname);
    } /* end of add_font() */

/* end of file */

