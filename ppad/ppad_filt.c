/*
** mouse:~ppr/src/ppad/ppad_filt.c
** Copyright 1995--2003, Trinity College Computing Center.
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
** Last modified 24 February 2003.
*/

/*
** This file contains the code which generates default filter options
** ("DefFiltOpts:") line for a printer or group configuration file.  It does 
** this by examining all of the PPD files (only one for a printer) and
** by trying to find a matching mode in mfmodes.conf.  The result will look 
** something like this:
**
** level=2 colour=False resolution=600 freevm=17000000 mfmode=ljfive
*/

#include "before_system.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "ppad.h"
#include "util_exits.h"

/* #define DEBUG_FILT 1 */

/*
** The variables we use the compile a "DefFiltOpts:" line.
*/
static int open_called = FALSE;		/* has deffiltopts_open() been called? */
static int level;			/* -1 if not seen yet, otherwise lowest */
static char *resolution = (char*)NULL;
static int resolution_conflict;		/* TRUE if not all are the same */
static int colour;			/* TRUE if any have colour yet */
static int freevm;			/* zero means not found yet, otherwise, smallest yet */
static char product[64];
static char modelname[64];
static char nickname[64];
static char *mfmode = (char*)NULL;
static int mfmode_conflict;

/*
** Given a Product, ModelName, Nickname, and resolution return
** a MetaFont mode name.
**
** The name will be returned in gu_alloc()ed memory.
*/
static char *get_mfmode(const char *product, const char *modelname, const char *nickname, const char *resolution)
    {
    const char filename[] = MFMODES;
    FILE *modefile;
    int line_space_available = 80;	/* suggested initial line buffer size */
    char *line = NULL;
    int linenum;
    const char *p, *m, *n, *r;		/* for cleaned up arguments */
    char *f[5];				/* for line fields */
    char *ptr;
    int x;
    char *answer = (char*)NULL;

    #ifdef DEBUG_FILT
    printf("get_mfmode(product=\"%s\", modelname=\"%s\", nickname=\"%s\"\n",
	product ? product : "<NULL>",
	modelname ? modelname : "<NULL>",
	nickname ? nickname : "<NULL>");
    #endif

    /* Assign short variable names and replace NULL pointers
       with zero-length strings. */
    p = product ? product : "";
    m = modelname ? modelname : "";
    n = nickname ? nickname : "";
    r = resolution ? resolution : "";

    if(debug_level >= 2)
    	printf(X_("Looking up mfmode for product \"%s\", modelname \"%s\", nickname \"%s\", resolution \"%s\".\n"), p, m, n, r);

    if((modefile = fopen(filename, "r")) == (FILE*)NULL)
	{
	fprintf(errors, _("%s: can't open \"%s\", errno=%d (%s)\n"), myname, filename, errno, gu_strerror(errno));
	return (char*)NULL;
	}

    for(linenum=1; (line = gu_getline(line, &line_space_available, modefile)); linenum++)
    	{
	/* Skip comments and blank lines.  gu_getline() has already removed trailing spaces. */
	if(line[0]==';' || line[0]=='#' || line[0]=='\0')
	    continue;

	ptr = line;
	if(!(f[0] = gu_strsep(&ptr, ":"))
		|| !(f[1] = gu_strsep(&ptr, ":"))
		|| !(f[2] = gu_strsep(&ptr, ":"))
		|| !(f[3] = gu_strsep(&ptr, ":"))
		|| !(f[4] = gu_strsep(&ptr, ":")))
	    {
	    fprintf(errors, _("Warning: syntax error in \"%s\" line %d: too few fields\n"), filename, linenum);
	    continue;
	    }

	for(x=0; x < (sizeof(f) / sizeof(f[0])); x++)
	    {
	    if(f[x][0] == '\0')
	    	{
	    	fprintf(errors, _("Warning: syntax error in \"%s\" line %d: field %d is empty\n"), filename, linenum, x);
	    	}
	    }
	if(x < (sizeof(f) / sizeof(f[0]))) continue;	/* skip line if error detected in the for() loop */

	if(debug_level >= 3)
	    printf(X_("line %d: product \"%s\", modelname \"%s\", nickname \"%s\", resolution \"%s\", mfmode \"%s\"\n"), linenum, f[0], f[1], f[2], f[3], f[4]);

	if(f[0][0] != '*' && strcmp(p, f[0]))
	    continue;

	if(f[1][0] != '*' && strcmp(m, f[1]))
	    continue;

	if(f[2][0] != '*' && strcmp(n, f[2]))
	    continue;

	if(f[3][0] != '*' && strcmp(r, f[3]))
	    continue;

	answer = gu_strdup(f[4]);

	if(debug_level >= 2)
	    printf(_("Match at \"%s\" line %d, mfmode=%s.\n"), filename, linenum, answer);

	gu_free(line);		/* didn't hit EOF so must do it ourselves */
	break;
    	}

    if(ferror(modefile))
	fprintf(errors, _("%s: error reading \"%s\", errno=%d (%s)\n"), myname, filename, errno, gu_strerror(errno));

    fclose(modefile);

    if(!answer)
	{
    	fprintf(errors, _("Warning: \"%s\" has no matching clause!  Some filters\n"
    			"\tmay not work correctly.\n"), filename);
	}

    return answer;
    } /* end of get_mfmode() */

/*
** This routine initializes the variables in order to
** get ready to read PPD files.
*/
void deffiltopts_open(void)
    {
    #ifdef DEBUG_FILT
    fputs("deffiltopts_open()\n", stdout);
    #endif

    if(debug_level >= 2)
    	printf("Gathering information for \"DefFiltOpts:\" line.\n");

    open_called = TRUE;			/* this routine has been called */
    level = -1;				/* unknown */
    resolution = (char*)NULL;		/* unknown */
    resolution_conflict = FALSE;	/* not yet */
    colour = FALSE;			/* assume false */
    freevm = 0;				/* unknown */

    mfmode = (char*)NULL;		/* unknown */
    mfmode_conflict = FALSE;		/* not yet */
    } /* end of deffiltopts_open() */

/*
** Read a PPD file and contribute its contents to
** the information we will use to supply a "DefFiltOpts:" line.
**
** Various PPAD sub-commands call this.  They call it when they
** copy the "PPDFile:" line.
*/
int deffiltopts_add_ppd(const char printer_name[], const char ppd_name[], const char *InstalledMemory)
    {
    int ret;
    char *line;				/* the line we are working on */
    char *p;				/* pointer into that line */
    int tempint;			/* integer for gu_sscanf() */
    char *tempptr;			/* char pointer for gu_sscanf() */
    int thisppd_resolution = FALSE;	/* <-- We use these variables in */
    int thisppd_level = FALSE;		/* order to honour only the */
    int thisppd_colour = FALSE;		/* first instance of certain */
    int thisppd_freevm = 0;		/* keywords. */
    int thisppd_product = FALSE;
    int thisppd_modelname = FALSE;
    int thisppd_nickname = FALSE;
    int vmoptions_count = 0;
    struct { char *opt; int val; } vmoptions[MAX_VMOPTIONS];

    #ifdef DEBUG_FILT
    printf("deffiltopts_add_ppd(\"%s\", \"%s\")\n", ppd_name, InstalledMemory ? InstalledMemory : "NULL");
    #endif

    if(debug_level > 1)
    	printf("Extracting deffiltopts information from PPD file \"%s\".\n", ppd_name);

    if( ! open_called )
    	fatal(EXIT_INTERNAL, "deffiltopts_add_ppd(): not open");

    product[0] = '\0';		/* clear metafont information */
    modelname[0] = '\0';
    nickname[0] = '\0';

    /* Open the PPD file. */
    if((ret = ppd_open(ppd_name, errors)))
	{
	#ifdef DEBUG_FILT
	printf("ppd_open() failed\n");
	#endif
    	return ret;
    	}

    /* Examine each line in the PPD file. */
    while((line = ppd_readline()))
    	{
	/* Short circuit comments, blank lines, and such: */
	if(line[0] != '*' || lmatch(line, "*%"))
	    continue;

	if(debug_level > 5)
	    printf("PPD: %s", line);

	/*
	** If this is a "*LanguageLevel:" line and either there has
	** been no previous such line or this is a lower level,
	** then accept this as the final level.
	*/
	if(!thisppd_level && gu_sscanf(line, "*LanguageLevel: \"%d\"", &tempint) == 1)
	    {
	    if(level == -1 || tempint < level)
	    	level = tempint;
	    thisppd_level = TRUE;
	    continue;
	    }

	/*
	** If this is the first "*DefaultResolution:" line in this file
	** and there has been no resolution conflict detected previously,
	*/
	if(!thisppd_resolution && ((p = lmatchp(line, "*DefaultResolution:")) || (p = lmatchp(line, "*DefaultJCLResolution:"))))
	    {
	    if(! resolution_conflict && gu_sscanf(p, "%S", &tempptr) == 1 )
		{
		/* Replace resolution variants like "600x600dpi" with things like "600dpi". */
		{
		char *p;
		if((p = strchr(tempptr, 'x')))
		    {
		    int nlen = (p - tempptr);
		    if(strncmp(tempptr, tempptr + nlen + 1, nlen) == 0 && strcmp(tempptr + nlen + nlen + 1, "dpi") == 0)
			{
			p = gu_strdup(tempptr + nlen + 1);
			gu_free(tempptr);
			tempptr = p;
			}
		    }
		}

		if(!resolution)			/* If no resolution statement in prior file, save this one. */
		    {
		    resolution = tempptr;
		    }
		else				/* If there was a resolution statement in prior file, */
		    {
		    /* If they disagree, declare a conflict. */
		    if(strcmp(resolution, tempptr) != 0)
			{
			gu_free(resolution);
			resolution = (char*)NULL;
			resolution_conflict = TRUE;
			}
		    gu_free(tempptr);
		    }
		}
	    thisppd_resolution = TRUE;
	    continue;
	    }

    	/*
    	** If this is a "*FreeVM:" line and it is the first one
    	** in this file, accept this new value.
    	*/
    	if(thisppd_freevm == 0 && gu_sscanf(line, "*FreeVM: \"%d\"", &tempint) == 1)
    	    {
	    thisppd_freevm = tempint;
    	    continue;
    	    }

	/*
	** If this is a "*ColorDevice:" line and it says true, set
	** colour to TRUE.
	*/
	if(!thisppd_colour && lmatch(line, "*ColorDevice:"))
	    {
	    tempptr = &line[13];
	    tempptr += strspn(tempptr," \t");
	    if( tempptr[0] == 'T' )
	    	colour = TRUE;
	    thisppd_colour = TRUE;
	    continue;
	    }

	/*
	** Collect *Product, *ModelName, and *NickName information
	** for determining the MetaFont mode.
	*/
	if(!thisppd_product && lmatch(line, "*Product:"))
	    {
	    tempptr = &line[9];
	    tempptr += strspn(tempptr, " \t\"(");
	    tempptr[strcspn(tempptr,")")] = '\0';
	    strncpy(product, tempptr, sizeof(product));	/* do a */
	    product[sizeof(product)-1] = '\0';	/* truncating copy */

	    thisppd_product = TRUE;
	    continue;
	    }
	if(!thisppd_modelname && lmatch(line, "*ModelName:"))
	    {
	    tempptr = &line[11];
	    tempptr += strspn(tempptr, " \t\"");
	    tempptr[strcspn(tempptr,"\"")] = '\0';
	    strncpy(modelname, tempptr, sizeof(modelname));	/* do a */
	    modelname[sizeof(modelname)-1] = '\0';	/* truncating copy */

	    thisppd_modelname = TRUE;
	    continue;
	    }
	if(!thisppd_nickname && lmatch(line, "*NickName:"))
	    {
	    tempptr = &line[10];
	    tempptr += strspn(tempptr," \t\"");
	    tempptr[strcspn(tempptr,"\"")] = '\0';
	    strncpy(nickname, tempptr, sizeof(nickname));	/* do a */
	    nickname[sizeof(nickname)-1] = '\0';		/* truncating copy */

	    thisppd_nickname = TRUE;
	    continue;
	    }

	/*
	** VMOption
	*/
	if(lmatch(line, "*VMOption"))
    	    {
	    char *ptr;
	    if(vmoptions_count >= MAX_VMOPTIONS)
	    	{
	    	fprintf(errors, "deffiltopts_add_ppd(): VMOptions overflow\n");
	    	}
	    else
	    	{
		ptr = &line[9];
		ptr += strspn(ptr, " \t");
		vmoptions[vmoptions_count].opt = gu_strndup(ptr, strcspn(ptr, "/:"));

		ptr += strcspn(ptr, ":");
		ptr += strspn(ptr, ": \t\"");
		gu_sscanf(ptr, "%d", &vmoptions[vmoptions_count].val);

		vmoptions_count++;
	    	}

    	    continue;
    	    }

    	} /* end of line reading loop */

    /*
    ** See if we can use VMOption information gathered above.
    */
    if(InstalledMemory)
    	{
	int x;

	for(x=0; x < vmoptions_count; x++)
	    {
	    #ifdef DEBUG_FILT
	    printf("vmoptions %d: %s %d\n", x, vmoptions[x].opt, vmoptions[x].val);
	    #endif

	    if(strcmp(InstalledMemory, vmoptions[x].opt) == 0)
	    	{
		#ifdef DEBUG_FILT
		printf("match!\n");
		#endif
		thisppd_freevm = vmoptions[x].val;
		break;
	    	}
	    }
    	}

    /*
    ** If this PPD file has yielded the lowest freevm
    ** value yet, then let it prevail.
    */
    if(freevm == 0 || thisppd_freevm < freevm)
    	freevm = thisppd_freevm;

    /*
    ** This will help explain the subsequent warning about the failed mfmode search.
    */
    if( ! thisppd_resolution && ! resolution_conflict )
	{
     	fprintf(errors, _("Warning: PPD file \"%s\" (for printer\n"
     	"\"%s\") does not have a \"*DefaultResolution:\" line.\n"), ppd_name, printer_name);
	}

    /*
    ** If we have not yet determined that not all the devices use the same 
    ** mfmode then see if this one uses the same mfmode as the others.
    **
    ** Notice that we skip this step if get_mfmode() fails.  There might be 
    ** a better way to handle such a failure.
    **
    ** Also notice that we don't even try if we didn't find a 
    ** "*DefaultResolution:" line in the PPD.  (The else takes care of that.)
    */
    else if(! mfmode_conflict)
    	{
    	if((tempptr = get_mfmode(product, modelname, nickname, resolution)))
	    {
            if(!mfmode)			/* If we are doing the 1st printer, */
                {			/* keep it without question. */
                mfmode = tempptr;
                }
            else			/* otherwise, */
                {
                if(strcmp(mfmode, tempptr) != 0)	/* if they don't match, */
                    {
                    gu_free(mfmode);			/* throw away both. */
                    mfmode = (char*)NULL;
                    mfmode_conflict = TRUE;
                    }
                gu_free(tempptr);
                }
	    }
	}

    #ifdef DEBUG_FILT
    printf("deffiltopts_add_ppd(): done\n");
    #endif

    return EXIT_OK;
    } /* end of deffiltopts_add_ppd() */

/*
** Determine the PPD file name for a printer and call
** deffiltopts_add_ppd() with that name.
**
** Various PPAD sub-commands that edit gropu files call this.  They
** call it each time they copy a "Printer:" line.
*/
int deffiltopts_add_printer(const char printer_name[])
    {
    char fname[MAX_PPR_PATH];
    FILE *f;
    char *line = NULL;
    int line_len = 128;
    char *PPDFile = (char*)NULL;
    char *InstalledMemory = (char*)NULL;
    int ret;

    #ifdef DEBUG_FILT
    printf("deffiltopts_add_printer(\"%s\")\n", printer_name);
    #endif

    ppr_fnamef(fname, "%s/%s", PRCONF, printer_name);
    if((f = fopen(fname, "r")) == (FILE*)NULL)
    	{
    	fprintf(errors, _("Member printer \"%s\" does not exist.\n"), printer_name);
    	return EXIT_BADDEST;
    	}

    /*
    ** Search the printer's configuration file to figure out which
    ** PPD file it uses and what installed memory option has been
    ** selected.
    */
    while((line = gu_getline(line, &line_len, f)))
    	{
	if(lmatch(line, "PPDFile:"))
	    {
	    char *ptr = &line[8];
	    int len = strlen(ptr);			/* remove */

	    while(len>0 && isspace(ptr[--len]))		/* trailing space */
	    	ptr[len] = '\0';
	    while( *ptr && isspace(*ptr) ) ptr++;	/* skip leading space */

	    if(PPDFile)
	    	{
		fprintf(errors, _("Warning: Ignoring all but last \"PPDFile:\" line for %s.\n"), printer_name);
	    	gu_free(PPDFile);
	    	}

	    PPDFile = gu_strdup(ptr);
	    }
	else if(lmatch(line, "PPDOpt: *InstalledMemory "))
	    {
	    if(InstalledMemory) gu_free(InstalledMemory);
	    InstalledMemory = gu_strndup(&line[25], strcspn(&line[25], " "));
	    }
    	}

    fclose(f);

    if(PPDFile)
	{
    	if((ret = deffiltopts_add_ppd(printer_name, PPDFile, InstalledMemory)) == EXIT_BADDEST)
    	    fprintf(errors, _("Printer \"%s\" has an invalid PPD file \"%s\".\n"), printer_name, PPDFile);
	gu_free(PPDFile);
	}
    else
	{
	fprintf(errors, _("Printer \"%s\" has no PPD file assigned.\n"), printer_name);
	ret = EXIT_BADDEST;
	}

    if(InstalledMemory) gu_free(InstalledMemory);

    #ifdef DEBUG_FILT
    printf("deffiltopts_add_printer(): done\n");
    #endif

    return ret;
    } /* end of deffiltopts_add_printer() */

/*
** Construct the key/value pairs for the "DefFiltOpts:" line.
*/
char *deffiltopts_line(void)
    {
    static char result_line[256];

    #ifdef DEBUG_FILT
    fputs("deffiltopts_line()\n", stdout);
    #endif

    if( ! open_called )
    	fatal(EXIT_INTERNAL, "deffiltopts_line(): not open");

    snprintf(result_line, sizeof(result_line), "level=%d colour=%s", level == -1 ? 1 : level, colour ? "True" : "False");

    if(resolution)
	gu_snprintfcat(result_line, sizeof(result_line), " resolution=%.*s", (int)strspn(resolution, "0123456789"), resolution);

    if(freevm != 0)
	gu_snprintfcat(result_line, sizeof(result_line), " freevm=%d", freevm);

    if(mfmode)
    	gu_snprintfcat(result_line, sizeof(result_line), " mfmode=%s", mfmode);

    #ifdef DEBUG_FILT
    printf("deffiltopts_line(): returning \"%s\"\n", result_line);
    #endif

    if(debug_level >= 2)
    	printf("Line is: %s\n", result_line);

    return result_line;
    } /* end of deffiltopts_line() */

/*
** This routine is here so we can free any allocated memory.
*/
void deffiltopts_close(void)
    {
    const char function[] = "deffiltopts_close";
    #ifdef DEBUG_FILT
    printf("%s()\n", function);
    #endif

    if( ! open_called )
    	fatal(EXIT_INTERNAL, "%s(): not open", function);

    if(resolution) gu_free(resolution);
    if(mfmode) gu_free(mfmode);

    open_called = FALSE;
    } /* end of deffiltopts_close() */

/* end of file */
