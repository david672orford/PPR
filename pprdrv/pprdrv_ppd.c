/*
** mouse:~ppr/src/pprdrv/pprdrv_ppd.c
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
** Last modified 15 June 2000.
*/

/*
** The functions in this file read the PPD file and later
** dispense information from it.
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

#include "global_structs.h"
#include "pprdrv.h"
#include "interface.h"

extern FILE *yyin;                  /* lex's input file */
int yylex(void);                    /* lex's principle function */

/*
** Globals
*/
struct PPDSTR **ppdstr;		/* PPD strings hash table */
struct PPDFONT **ppdfont;	/* PPD fontlist hash table */
char *ppdname;			/* name of next PPD string */
char *ppdtext;			/* text of next PPD string */

char *ppd_nest_fname[MAX_PPD_NEST];	/* names of all nested PPD files */
int ppd_nest_level;			/* number of PPD files now open */

int tindex;
struct FEATURES Features;			/* list of features */
struct PAPERSIZE papersize[MAX_PAPERSIZES];
int papersizex = 0;				/* index into papersize[] */
int num_papersizes = 0;

/*
** Hash functions:
**
** Hash the string s, tabsize is the size of the hash
** table.  The return value will be less than tabsize.
*/
static int hash(const char *s, int tabsize)
    {
    unsigned n=0;           /* intialize the work value */

    while(*s)               /* hash the string */
	n = 32 * n + *s++;
    return n % tabsize;     /* wrap value and return it */
    } /* end of hash() */

/*
** Hash two strings as though they were one.
**
** s2 may be a NULL pointer.
*/
static int hash2(const char *s1, const char *s2, int tabsize)
    {
    unsigned n=0;           /* initialize the work value */

    while(*s1)              /* hash the 1st part */
	n = 32 * n + *s1++;

    if(s2)
    	{
    	if(*s2)			/* if second part is not empty, */
	    n = 32 * n + ' ';	/* hash in a space */
	while(*s2)		/* if 2nd part not empty, hash it too */
	    n = 32 * n + *s2++;
	}

    return n % tabsize;     /* return wraped result */
    } /* end of hash2() */

/*=========================================================
** Functions called by the lexer.
=========================================================*/
/*
** add_font() is called for each *Font: line.
** The hash structure used by this routine and find_font() is
** different from that used by the others in this module.
*/
void add_font(char *fontname)
    {
    int h;
    struct PPDFONT *p;

    DODEBUG_PPD(("add_font(\"%s\")", fontname ? fontname : "<NULL>"));

    p = (struct PPDFONT*)gu_alloc(1,sizeof(struct PPDFONT));
    p->name = gu_strdup(fontname);

    h = hash(fontname,FONT_TABSIZE);
    p->next = ppdfont[h];
    ppdfont[h] = p;
    }

/*
** This is called by the lexer when it detects the start of
** a new string.  The argument is cleaned up and stored in
** ppdname[] until we are ready to use it.
*/
void new_string(const char name[])
    {
    int c;                              /* next character */
    int x = 0;				/* index into string */

    DODEBUG_PPD(("new_string(name=\"%s\")", name ? name : "<NULL>"));

    while(*name && x < MAX_PPDNAME)
	{
	switch(c = *(name++))
	    {
	    case '/':			/* if slash (translate string intro) */
	    case ':':			/* or colon (value seperator) */
		name = "";		/* stop things */
		break;
	    case ' ':			/* if space, */
		while(*name == ' ')	/* eat up multiples */
		    name++;		/* and fall thru */
	    default:
		ppdname[x++] = c;
	    }
	}

    ppdname[x] = '\0';		/* terminate it */
    tindex = 0;			/* initialize text buffer index */
    ppdtext[0] = '\0';
    } /* end of new_string() */

/*
** The lexer calls this each time it reads a line of the string value.
** It appends the line to ppdtext for later storage in the hash table.
**
** The argument is the line of string data that has been read.
*/
void string_line(char *string)
    {
    const char function[] = "string_line";
    int string_len;

    #ifdef DEBUG_PPD_DETAILED
    debug("string_line(\"%s\")", string ? string : "<NULL>");
    #endif

    if(tindex != 0 && tindex != MAX_PPDTEXT)	/* if second or later line, */
	ppdtext[tindex++] = '\n';		/* seperate with line feed */

    string_len = strlen(string);

    if( (MAX_PPDTEXT - tindex) < (string_len + 1) )
	fatal(EXIT_PRNERR_NORETRY, "%s(): text too long", function);

    while(*string)				/* append this line */
	ppdtext[tindex++] = *string++;

    ppdtext[tindex] = '\0';			/* terminate but don't advance */
    } /* end of string_line() */

/*
** All of the string is found, put it in the hash table.
*/
void end_string(void)
    {
    struct PPDSTR *s;
    struct PPDSTR **p;
    int h;

    #ifdef DEBUG_PPD_DETAILED
    debug("end_string()");
    #endif

    s = (struct PPDSTR*)gu_alloc(1, sizeof(struct PPDSTR));
    s->name = gu_strdup(ppdname);		/* duplicate the temp values */
    s->value = gu_strdup(ppdtext);		/* and store ptrs to them */

    h = hash(ppdname, PPD_TABSIZE);		/* hash the name */

    p = &ppdstr[h];				/* get pointer to 1st pointer */
    while( *p != (struct PPDSTR *)NULL )	/* search for the null one */
	p = &((*p)->next);
    *p = s;					/* set it to point to new entry */
    s->next = (struct PPDSTR *)NULL;		/* and nullify its next pointer */
    } /* end of end_string() */

/*
** Process *OrderDependency information.
*/
void order_dependency_1(int order)
    {


    } /* end of of order_dependency_1() */

void order_dependency_2(int section)
    {


    } /* end of of order_dependency_2() */

void order_dependency_3(const char *name1)
    {


    } /* end of of order_dependency_3() */

void order_dependency_4(const char *name2)
    {


    } /* end of of order_dependency_4() */

/*
** Move the paper size array index.
*/
void papersize_moveto(char *nameline)
    {
    struct PAPERSIZE *p;
    char name[32];
    char *ptr;
    int len;

    ptr=&nameline[strcspn(nameline," \t")];	/* extract the */
    ptr+=strspn(ptr," \t");			/* PageSize name */
    len=strcspn(ptr," \t:/");			/* from the line */
    len=len<=31?len:31;				/* truncate if necessary */
    sprintf(name, "%.*s", len, ptr);

    DODEBUG_PPD(("papersize_moveto(\"%s\")", name ? name : "<NULL>"));

    for(papersizex = 0; papersizex < num_papersizes; papersizex++)
    	{
    	if(strcmp(papersize[papersizex].name, name) == 0)
    	    return;
    	}

    if( (papersizex=num_papersizes++) >= MAX_PAPERSIZES )
    	fatal(EXIT_PRNERR_NORETRY, "pprdrv: MAX_PAPERSIZES is not large enough");

    p = &papersize[papersizex];
    p->name = gu_strdup(name);
    p->width = p->height = p->lm = p->tm = p->rm = p->bm = 0;
    } /* end of papersize_moveto() */

/*==========================================================
** Read the Adobe PostScript Printer Description file.
==========================================================*/
void read_PPD_file(const char *ppd_file_name)
    {
    int x;              /* used to initialize structures */

    /* Set the default values. */
    Features.ColorDevice = FALSE;
    Features.Extensions = 0;
    Features.FaxSupport = FAXSUPPORT_NONE;
    Features.FileSystem = FALSE;
    Features.LanguageLevel = 1;
    Features.TTRasterizer = TT_UNKNOWN;
    printer.prot.TBCP = FALSE;
    printer.prot.PJL = FALSE;

    /* Create hash tables for code strings and fonts. */
    ppdstr = (struct PPDSTR**)gu_alloc(PPD_TABSIZE, sizeof(struct PPDSTR*));
    ppdfont = (struct PPDFONT**)gu_alloc(FONT_TABSIZE, sizeof(struct PPDFONT*));

    for(x=0; x < PPD_TABSIZE; x++)
	ppdstr[x] = (struct PPDSTR *)NULL;
    for(x=0; x < FONT_TABSIZE; x++)
	ppdfont[x] = (struct PPDFONT *)NULL;

    /*
    ** A printer configuration file is not absolutely required to name
    ** a PPD file.  (Though the ppad command makes it pretty diffucult
    ** not to.  You have to hand create or edit the printer configuration
    ** file.  Obviously, this code was inserted in the early days of PPR.)
    ** If printer has no PPD file, then stop things right here before we
    ** get to fopen(), which might cause a core dump.
    */
    if(ppd_file_name == (const char *)NULL)
    	return;

    /*
    ** Allocate temporary storage for the current name
    ** and the current code string.
    */
    ppdname = (char*)gu_alloc(MAX_PPDNAME+1, sizeof(char));
    ppdtext = (char*)gu_alloc(MAX_PPDTEXT, sizeof(char));

    /* open the PPD file */
    if((yyin = fopen(ppd_file_name, "r")) == (FILE*)NULL)
	fatal(EXIT_PRNERR_NORETRY, "%s: read_PPD_file(): can't open \"%s\", errno=%d (%s)", __FILE__, ppd_file_name, errno, gu_strerror(errno));

    ppd_nest_level = 0;
    ppd_nest_fname[ppd_nest_level] = gu_strdup(ppd_file_name);	/* !!! */

    /* call the lexer to process the file */
    yylex();

    /* Now, close the outermost file, but DON'T deallocate the name: */
    fclose(yyin);

    /* Free the scratch spaces: */
    gu_free(ppdname);
    gu_free(ppdtext);
    } /* read_PPD_file() */

/*==========================================================
** Routines for feature inclusion.
==========================================================*/

/*
** Add a warning to the log file about a printer feature
** for which we did not have code.
*/
static void feature_warning(const char *format, ...)
    {
    char fname[MAX_PPR_PATH];
    FILE *log;
    va_list va;

    ppr_fnamef(fname, "%s/%s-log", DATADIR, QueueFile);

    if((log = fopen(fname, "a")))
	{
	va_start(va, format);
	vfprintf(log, format, va);
	va_end(va);
	fputc('\n', log);
	fclose(log);
	}
    } /* end of feature_warning() */

/*
** Return a pointer to feature code.
** If the requested feature is not available,
** return a NULL pointer.
*/
const char *find_feature(const char *featuretype, const char *option)
    {
    struct PPDSTR *s;			/* pointer to PPD database entry */
    int len;				/* used in comparison */
    char *string = (char*)NULL;		/* value to return */

    DODEBUG_PPD(("find_feature(\"%s\", \"%s\")", featuretype, option ? option : ""));

    s = ppdstr[hash2(featuretype, option, PPD_TABSIZE)];

    while(s)
	{
	if( (strncmp(s->name,featuretype,len=strlen(featuretype))==0)
	    && ( ( option==(char*)NULL && s->name[len]=='\0' )
		|| ( s->name[len]==' ' && strcmp(&s->name[len+1],option)==0 ) )
	    ) /* <-- notice parenthesis */
	    {
	    string = s->value;			/* use the value */
	    break;
	    }
	else					/* if no match */
	    {					/* if chain continues, */
	    s = s->next;			/* jump to next link */
	    }
	}

    return string;
    } /* end of find_feature() */

/*
** Insert feature code, if we have it.  If not, insert an
** "%%IncludeFeature:" comment so that a spooler furthur down
** the line can insert it if it has it.
**
** This function is called whenever an ``%%IncludeFeature:'' comment
** is encountered in the input file.  It is also called by
** insert_features() (see below).
*/
void include_feature(const char *featuretype, const char *option)
    {
    const char *function = "include_feature";
    const char *string;

    DODEBUG_PPD(("%s()", function));

    /* Ignore %%IncludeFeature: without arguments */
    if(!featuretype)
	return;

    /* Check if I guessed wrong about interpretion of 2nd argument.
       (It seems a non-NULL but empty option is not the right way to
       represent a missing option.) */
    if(option && !option[0])
    	fatal(EXIT_PRNERR_NORETRY, "%s(): assertion failed", function);

    /*
    ** If we are removing binselects, then don't copy them and comment
    ** out certain features.
    */
    if(strip_binselects)
    	{
	if(strcmp(featuretype, "*InputSlot") == 0 || strcmp(featuretype, "*TraySwitch") == 0)
	    {
	    #ifdef KEEP_OLD_CODE
	    printer_printf("%% %%%%IncludeFeature: %s %s\n", featuretype, option ? option : "");
	    #endif
	    return;
	    }

	if(strcmp(featuretype, "*PageSize") == 0)	/* change *PageSize */
	    {
	    #ifdef KEEP_OLD_CODE
	    printer_printf("%% %%%%IncludeFeature: *PageSize %s\n", option ? option : "");
	    #endif
	    featuretype = "*PageRegion";	/* to *PageRegion */
	    }
	}

    /*
    ** This is used to strip out signature and
    ** booklet mode invokation code if PPR is doing the job.
    */
    if(strip_signature && ( (strcmp(featuretype,"*Signature")==0) || (strcmp(featuretype,"*Booklet")==0) ) )
	{
	#ifdef KEEP_OLD_CODE
	printer_printf("%% %%%%IncludeFeature: %s %s\n", featuretype, option ? option : "");
	#endif
	return;
	}


    /* The feature code is available, */
    if((string = find_feature(featuretype, option)))
	{
	/* Anounce the feature start. */
	if(option)
	    printer_printf("%%%%BeginFeature: %s %s\n", featuretype, option);
	else
	    printer_printf("%%%%BeginFeature: %s\n", featuretype);

	/* Include the PostScript code to invoke the feature. */
	if(option)
	    printer_puts(string);
	else					/* no option keyword, convert QuotedValue */
	    printer_puts_QuotedValue(string);	/* to binary and write it to the printer. */

	/* and flag its end */
	printer_puts("\n%%EndFeature\n");
	}

    /*
    ** If the feature code is was not found in the PPD file.
    */
    else
	{
	/*
	** If we are using N-Up and it is a *PageRegion
	** feature, guess at the correct code.
	**
	** We do this because the N-Up machinery may be able
	** to interpret many commands that the printer can't.
	*/
	if(job.N_Up.N != 1 && strcmp(featuretype,"*PageRegion") == 0 && option)
	    {
	    char *ptr = gu_strdup(option);
	    printer_printf("%%%%BeginFeature: *PageRegion %s\n", option);
	    while(ptr)
	        {
	        *ptr = tolower(*ptr);
	        ptr++;
	        }
	    printer_puts(option);
	    printer_puts("\n%%EndFeature\n");
	    gu_free(ptr);
	    }

	/*
	** As a last ditch effort, just insert an "%%IncludeFeature:" comment.
	*/
	else
	    {
	    feature_warning(_("The PPD file doesn't contain code for the printer feature\n"
	    	"called \"%s %s\", skipping it."), featuretype, option ? option : "");

	    if(option)
		printer_printf("%%%%IncludeFeature: %s %s\n",featuretype,option);
	    else
		printer_printf("%%%%IncludeFeature: %s\n",featuretype);
	    }
	}

    } /* end of include_feature() */

/*
** Call this function when we have "%%BeginFeature: ..." in line[].
**
** Generally, the "%%BeginFeature:" line is written out and the feature is
** looked up in the table we built from the PPD file.  If it is found in the
** PPD file table, the code from the PPD file is used to replace the origional
** code, the origional code is discarded.
**
** If the feature is not in the PPD file, then the origional code may be
** retailed (if -K true).
**
** The second parameter (option) may be a NULL pointer.
*/
void begin_feature(char *featuretype, char *option, FILE *infile)
    {
    const char *function = "begin_feature";
    const char *string;
    gu_boolean fallback;		/* keep old code if PPD doesn't have it? */

    DODEBUG_PPD(("%s()", function));

    /* Ignore blank feature lines. */
    if(!featuretype)
    	return;

    /* Start with -K setting */
    fallback = job.opts.keep_badfeatures;

    /*
    ** If this is bin select code and we are stripping bin select code, or
    ** this is signature mode code and we are stripping signature code, then
    ** read until "%%EndFeature" and return.  If KEEP_OLD_CODE is defined
    ** then the whole block (including "%%BeginFeature:" and "%%EndFeature")
    ** is copied through but "% " is prepended to each line.
    */
    if( ( strip_binselects
 		&& ( (strcmp(featuretype, "*InputSlot") == 0)
 		|| (strcmp(featuretype, "*TraySwitch") == 0) ) )
 	|| ( strip_signature
 		&& ( (strcmp(featuretype, "*Signature") == 0)
 		|| (strcmp(featuretype, "*Booklet") == 0) ) )
 	)
	{
	#ifdef KEEP_OLD_CODE
	printer_printf("%% %s\n",line);		/* print "%%BeginFeature:" line commented out */
	#endif

	/* Swallow the rest of the feature block. */
	while(TRUE)
	    {
	    if(dgetline(infile) == (char*)NULL)
		{
		fatal(EXIT_JOBERR, "%s(): unterminated feature code", function);
		give_reason("defective feature invokation");
		}

	    #ifdef KEEP_OLD_CODE
	    printer_printf("%% %s\n", line);	/* Print old line, commented out. */
	    #endif

	    if(strcmp(line, "%%EndFeature") == 0)
		break;
	    }
	return;
	}

    /*
    ** This is where we write the "%%BeginFeature:" line if the block above
    ** didn't do it and return.
    **
    ** If we are stripping bin select code, then we must change "*PaperSize"
    ** features to "*PageRegion" features, otherwise they will change the
    ** bin selection on some printers.
    */
    if(strip_binselects && (strcmp(featuretype, "*PageSize") == 0))
	{
	#ifdef KEEP_OLD_CODE
	printer_printf("%% %s\n", line);
	#endif
	featuretype = "*PageRegion";
	printer_printf("%%%%BeginFeature: *PageRegion %s\n", option ? option : "");
	fallback = FALSE;		/* Don't fall back to old code, it is wrong! */
	}
    else
	{
	printer_putline(line);
	}

    /*
    ** If duplex code is not in the PPD file then the printer does
    ** not support duplex, so we want the code stript out even
    ** if the -K true switch was used with PPR.
    **
    ** I cannot remember a good reason for this code.  Since it makes
    ** the -K switch's effect more complicated, I have removed it.
    */
    #if 0
    if(strcmp(featuretype, "*Duplex") == 0)
	fallback = FALSE;
    #endif

    /*
    ** If the PPD file supplies PostScript code to invoke the requested
    ** printer feature or fallback is FALSE, eat the code from here `til
    ** the "%%EndFeature" comment.  Then, insert the code from the PPD file
    ** (if any) and write the "%%EndFeature" comment.
    */
    if((string = find_feature(featuretype, option)) || !fallback)
	{
	if(!string)
	    {
	    feature_warning(_("The PPD file doesn't contain code for the printer feature\n"
	    	"called \"%s %s\", removing existing code."), featuretype, option ? option : "");

	    printer_putline("% The code for this printer feature was removed by PPR because\n"
 	    		    "% there is no such feature listed in the PPD file.  Use ppr's -K\n"
 	    		    "% switch to change this behavior.");
	    }

	/* Comment out the old code */
	while(TRUE)
	    {
	    if(dgetline(infile) == (char*)NULL)
		{
		fatal(EXIT_JOBERR, "%s(): unterminated feature code", function);
		give_reason("defective feature invokation");
		}

	    if(strcmp(line, "%%EndFeature") == 0)
		break;

	    /* Retain old line commented out. */
	    #ifdef KEEP_OLD_CODE
	    printer_printf("%% %s\n", line);
	    #endif
	    }

	/*
	** If the PPD file has code for this, insert it.  If there is an option
	** it is just straight text.  If there is no option, then it is what
	** the PPD spec calls a QuotedValue and must be converted before use.
	*/
	if(string)
	    {
	    if(option)
		printer_puts(string);
	    else
		printer_puts_QuotedValue(string);

	    printer_putc('\n');
	    }

	/* and flag the end */
	printer_putline("%%EndFeature");
	}

    /*
    ** If we don't have PPD file code for this and we have determined that we
    ** should fall back to the code in the file, just copy it and the
    ** "%%EndFeature" comment.
    */
    else
	{
	feature_warning(_("The PPD file doesn't contain code for the printer feature\n"
		"called \"%s %s\", retaining old code."), featuretype, option ? option : "");

	printer_puts("% PPR retained the following feature code despite fact that no such\n"
		     "% feature is listed in the PPD file.  Use ppr's -K switch to change\n"
		     "% this behavior.\n");

	/* Copy it all through.  This is the only such loop in which we could
	   get away with just doing break and not fatal() on EOF, but we don't.
	   */
	while(TRUE)
	    {
	    if(dgetline(infile) == (char*)NULL)
		{
		fatal(EXIT_JOBERR, "%s(): unterminated feature code", function);
		give_reason("defective feature invokation");
		}

	    printer_putline(line);

	    if(strcmp(line, "%%EndFeature") == 0)
		break;
	    }
	}

    } /* end of begin_feature() */

/*
** PostScript stopped context.
**
** These routines emmit PostScript code which surrounds any feature
** invokation that we insert.  This bracketing code catches errors
** so that if, say, the duplex command failes, the job will still
** print.
*/
void begin_stopped(void)
    {
    printer_putline("[ { %PPR");
    } /* end of begin_stopped() */

void end_stopped(const char *feature, const char *option)
    {
    printer_printf("} stopped {(PPD error: %s %s failed\\n)print} if cleartomark %%PPR\n",feature,option);
    } /* end of end_stopped() */

/*
** Insert one feature for each "Feature:" line read from qstream.
** This is called twice.  When it is called with set==1, is will
** insert all code which does not set the duplex mode, when it is
** called with set==2, it will insert just the duplex setting code.
**
** This must be called with set==1 before it is called with set==2.
**
** Most of the features inserted by this routine will have been
** requested with the "ppr -F" switch or as a result of "ppr -R duplex"
** automatic duplexing.
*/
void insert_features(FILE *qstream, int set)
    {
    static char *duplex_code = (char*)NULL;
    char fline[256];
    char featuretype[32];
    char option[32];
    gu_boolean set_strip_binselects = FALSE;

    if(set==2)		/* If this is set 2, insert the code for the */
	{		/* remembered duplex feature. */
	if(duplex_code)
	    {
	    printer_putline("{currentpoint} stopped %PPR");
	    printer_putline("[0 0 0 0 0 0] currentmatrix %PPR");
	    begin_stopped();
	    include_feature("*Duplex", duplex_code);
	    end_stopped("*Duplex", duplex_code);
	    printer_putline("setmatrix %PPR");
	    printer_putline("not {moveto} if %PPR");
	    gu_free(duplex_code);
	    duplex_code = (char *)NULL;
	    }
	return;
	}

    if(duplex_code)
    	fatal(EXIT_PRNERR_NORETRY, "insert_features(): assertion failed");

    while(fgets(fline, sizeof(fline), qstream))
	{                   /* work until end of file */
	option[0] = '\0';

	if(strcmp(line, "EndSetups") == 0)
	    break;

	if(gu_sscanf(fline, "Feature: %#s %#s",
			    sizeof(featuretype), featuretype,
			    sizeof(option), option) == 0)
	    continue;       /* skip non-"Feature:" lines */

	/*
	** If the code to be inserted is a duplex feature command,
	** remember it for insertion during pass 2, otherwise,
	** just insert it now.
	*/
	if(strcmp(featuretype, "*Duplex") == 0)
	    {
	    duplex_code = gu_strdup(option);
	    }
	else
	    {
	    begin_stopped();
	    include_feature(featuretype, option);
	    end_stopped(featuretype, option);

	    /*
	    ** New as of 1.30:  Explicitly selecting the input slot causes
	    ** old bin select code to be stript out and PageSize code
	    ** to be turned into PageRegion code.
	    */
	    if(strcmp(featuretype, "*InputSlot") == 0)
	    	set_strip_binselects = TRUE;
	    }
	} /* end of while loop which itemizes features to insert */

    if(set_strip_binselects)
    	strip_binselects = TRUE;

    } /* end of insert_features() */

/*================================================================
** ppd_find_font() is called from pprdrv_res.c and pprdrv_capable.c.
** ppd_find_font() returns non-zero if it can't find the font
** in the list from the PPD file.
================================================================*/
gu_boolean ppd_font_present(const char fontname[])
    {
    int h;
    struct PPDFONT *p;              /* pointer to font structure */

    h = hash(fontname, FONT_TABSIZE);
    p = ppdfont[h];

    while(p != (struct PPDFONT *)NULL)
	{
	if(strcmp(fontname, p->name) == 0)
	    return TRUE;
	p = p->next;
	}

    return FALSE;
    } /* end of ppd_font_present() */

/* end of file */

