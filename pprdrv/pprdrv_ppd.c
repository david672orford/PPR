/*
** mouse:~ppr/src/pprdrv/pprdrv_ppd.c
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
** Last modified 21 July 2003.
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
#include <stdlib.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "global_structs.h"
#include "pprdrv.h"
#include "interface.h"
#include "pprdrv_ppd.h"
#include "util_exits.h"

static struct PPDSTR **ppdstr;			/* PPD strings hash table */
static struct PPDFONT **ppdfont;		/* PPD fontlist hash table */

/* We use these when receiving feature code from the lexer. */
static char *ppdname;					/* name of next PPD string */
static char *ppdtext;					/* text of next PPD string */
static int tindex;						/* index of next free byte in ppdtext */
static gu_boolean instring = FALSE;

/* This is for handling included PPD files. */
char *ppd_nest_fname[MAX_PPD_NEST];		/* names of all nested PPD files */
int ppd_nest_level;						/* number of PPD files now open */

/* */
struct FEATURES Features;

/* */
struct PAPERSIZE papersize[MAX_PAPERSIZES];
int papersizex = 0;								/* index into papersize[] */
int num_papersizes = 0;

/*=========================================================
** Hash Functions
=========================================================*/

/*
**
** Hash the string s, tabsize is the size of the hash
** table.  The return value will be less than tabsize.
*/
static int hash(const char *s, int tabsize)
	{
	unsigned n=0;			/* intialize the work value */

	while(*s)				/* hash the string */
		n = 32 * n + *s++;
	return n % tabsize;		/* wrap value and return it */
	} /* end of hash() */

/*
** Hash two strings as though they were one.
**
** s2 may be a NULL pointer.
*/
static int hash2(const char *s1, const char *s2, int tabsize)
	{
	unsigned n=0;			/* initialize the work value */

	while(*s1)				/* hash the 1st part */
		n = 32 * n + *s1++;

	if(s2)
		{
		if(*s2)					/* if second part is not empty, */
			n = 32 * n + ' ';	/* hash in a space */
		while(*s2)				/* if 2nd part not empty, hash it too */
			n = 32 * n + *s2++;
		}

	return n % tabsize;		/* return wraped result */
	} /* end of hash2() */

/*=========================================================
** Callback Functions for the Lexer
=========================================================*/

/*
** ppd_callback_add_font() is called for each "*Font:" line.
** The hash structure used by this routine and find_font() is
** different from that used by the others in this module.
*/
void ppd_callback_add_font(char *fontname)
	{
	FUNCTION4DEBUG("ppd_callback_add_font")
	int h;
	struct PPDFONT *p;

	DODEBUG_PPD(("%s(\"%s\")", function, fontname ? fontname : "<NULL>"));

	p = (struct PPDFONT*)gu_alloc(1, sizeof(struct PPDFONT));
	p->name = gu_strdup(fontname);

	h = hash(fontname,FONT_TABSIZE);
	p->next = ppdfont[h];
	ppdfont[h] = p;
	} /* end of ppd_callback_add_font() */

/*
** This is called by the lexer when it detects the start of
** a new string.  The argument is cleaned up and stored in
** ppdname[] until we are ready to use it.
*/
void ppd_callback_new_string(const char name[])
	{
	FUNCTION4DEBUG("ppd_callback_new_string")
	int c;								/* next character */
	int x = 0;							/* index into string */

	DODEBUG_PPD(("%s(name=\"%s\")", function, name ? name : "<NULL>"));

	while(*name && x < MAX_PPDNAME)
		{
		switch(c = *(name++))
			{
			case '/':					/* if slash (translate string intro) */
			case ':':					/* or colon (value seperator) */
				name = "";				/* stop things */
				break;
			case ' ':					/* if space, */
				while(*name == ' ')		/* eat up multiples */
					name++;				/* and fall thru */
			default:
				ppdname[x++] = c;
			}
		}

	ppdname[x] = '\0';			/* terminate it */
	tindex = 0;					/* initialize text buffer index */
	ppdtext[0] = '\0';
	instring = TRUE;
	} /* end of ppd_callback_new_string() */

/*
** The lexer calls this each time it reads a line of the string value.
** It appends the line to ppdtext for later storage in the hash table.
**
** The argument is the line of string data that has been read.
*/
void ppd_callback_string_line(char *string)
	{
	const char function[] = "ppd_callback_string_line";
	int string_len;

	DODEBUG_PPD_DETAILED(("%s(\"%s\")", function, string ? string : "<NULL>"));

	if(tindex != 0 && tindex != MAX_PPDTEXT)	/* if second or later line, */
		ppdtext[tindex++] = '\n';				/* seperate with line feed */

	string_len = strlen(string);

	if( (MAX_PPDTEXT - tindex) < (string_len + 1) )
		fatal(EXIT_PRNERR_NORETRY, "%s(): text too long", function);

	while(*string)								/* append this line */
		ppdtext[tindex++] = *string++;

	ppdtext[tindex] = '\0';						/* terminate but don't advance */
	} /* end of ppd_callback_string_line() */

/*
** All of the string is found, put it in the hash table.
*/
void ppd_callback_end_string(void)
	{
	FUNCTION4DEBUG("ppd_callback_end_string")
	struct PPDSTR *s;
	struct PPDSTR **p;
	int h;

	DODEBUG_PPD_DETAILED(("%s()", function));

	s = (struct PPDSTR*)gu_alloc(1, sizeof(struct PPDSTR));
	s->name = gu_strdup(ppdname);				/* duplicate the temp values */
	s->value = gu_strdup(ppdtext);				/* and store ptrs to them */

	h = hash(ppdname, PPD_TABSIZE);				/* hash the name */

	p = &ppdstr[h];								/* get pointer to 1st pointer */
	while( *p != (struct PPDSTR *)NULL )		/* search for the null one */
		p = &((*p)->next);
	*p = s;										/* set it to point to new entry */
	s->next = (struct PPDSTR *)NULL;			/* and nullify its next pointer */
	
	instring = FALSE;
	} /* end of ppd_callback_end_string() */

/*
** Process *OrderDependency information.
*/
void ppd_callback_order_dependency(const char text[])
	{

	} /* end of ppd_callback_order_dependency() */

/*
** Move the paper size array index.
*/
void ppd_callback_papersize_moveto(char *nameline)
	{
	struct PAPERSIZE *p;
	char name[32];
	char *ptr;
	int len;

	ptr = &nameline[strcspn(nameline," \t")];	/* extract the */
	ptr += strspn(ptr," \t");					/* PageSize name */
	len = strcspn(ptr," \t:/");					/* from the line */
	snprintf(name, sizeof(name), "%.*s", len, ptr);

	DODEBUG_PPD(("papersize_moveto(\"%s\")", name ? name : "<NULL>"));

	for(papersizex = 0; papersizex < num_papersizes; papersizex++)
		{
		if(strcmp(papersize[papersizex].name, name) == 0)
			return;
		}

	if((papersizex = num_papersizes++) >= MAX_PAPERSIZES)
		fatal(EXIT_PRNERR_NORETRY, "pprdrv: MAX_PAPERSIZES is not large enough");

	p = &papersize[papersizex];
	p->name = gu_strdup(name);
	p->width = p->height = p->lm = p->tm = p->rm = p->bm = 0;
	} /* end of ppd_callback_papersize_moveto() */

/*
** Process "*pprRIP:" information.
*/
void ppd_callback_rip(const char text[])
	{
	DODEBUG_PPD(("ppd_callback_rip(\"%s\")", text));
	if(!printer.RIP.name)		/* if first in PPD file and not set in printer config file */
		{
		char *p;

		/* Duplicate the Lex buffer because we are going to edit it in place
		   and keep pointers to the edited copy. */
		p = gu_strdup(text);

		/* Parse it using gu_strsep(), keeping pointers and inserting nulls. */
		if(!(printer.RIP.name = gu_strsep(&p, " \t")) || !(printer.RIP.output_language = gu_strsep(&p, " \t")))
			fatal(EXIT_PRNERR_NORETRY, _("Can't parse RIP information in PPD file."));
		printer.RIP.options_storage = gu_strsep(&p, "");

		if(strchr(printer.RIP.name, '/'))
			fatal(EXIT_PRNERR_NORETRY, _("Slashes are not allowed in RIP names in \"*pprRIP:\" lines in PPD files."));

		}
	} /* end of ppd_callback_rip() */

static char *cups_filter = NULL;
static int default_resolution = 0;

/*
** Process "*cupsFilter:" information.
*/
void ppd_callback_cups_filter(const char text[])
	{
	DODEBUG_PPD(("ppd_callback_cups_filter(\"%s\")", text));
	if(!cups_filter)
		{
		char *temp, *p, *p1, *p2, *p3;
		p = temp = gu_strdup(text);

		if((p1 = gu_strsep(&p, " \t"))									/* first exists */
				&& strcmp(p1, "application/vnd.cups-raster") == 0		/* and mime type matches */
				&& (p2 = gu_strsep(&p, " \t"))							/* second exists */
				&& strspn(p2, "0123456789") == strlen(p2)				/* and is numberic */
				&& (p3 = gu_strsep(&p, "\t"))							/* third exists */
			)
			{
			cups_filter = gu_strdup(p3);
			}
		gu_free(temp);
		}
	DODEBUG_PPD(("ppd_callback_cups_filter(): done"));
	}

/*
** Process "*DefaultResolution:".
*/
void ppd_callback_resolution(const char text[])
	{
	DODEBUG_PPD(("ppd_callback_resolution(\"%s\")", text));
	default_resolution = atoi(text);
	}

/*==========================================================
** Read the Adobe PostScript Printer Description file.
==========================================================*/

void read_PPD_file(const char *ppd_file_name)
	{
	const char function[] = "read_PPD_file";

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
	{
	int x;
	ppdstr = (struct PPDSTR**)gu_alloc(PPD_TABSIZE, sizeof(struct PPDSTR*));
	for(x=0; x < PPD_TABSIZE; x++)
		ppdstr[x] = (struct PPDSTR *)NULL;

	ppdfont = (struct PPDFONT**)gu_alloc(FONT_TABSIZE, sizeof(struct PPDFONT*));
	for(x=0; x < FONT_TABSIZE; x++)
		ppdfont[x] = (struct PPDFONT *)NULL;
	}
	
	/*
	** If the printer configuration does not specify a PPD file, bail out.  A
	** printer configuration file is not absolutely required to specify a PPD
	** file.  (Though the ppad command makes it pretty difficult not to.  You
	** have to hand create or edit the printer configuration file.  Obviously,
	** this code was inserted in the early days of PPR.)  If printer has no PPD file, then stop things right here before we
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

	#if 0

	/* open the PPD file */
	if(!(yyin = fopen(ppd_file_name, "r")))
		fatal(EXIT_PRNERR_NORETRY, "%s(): can't open \"%s\", errno=%d (%s)", function, ppd_file_name, errno, gu_strerror(errno));

	ppd_nest_level = 0;
	ppd_nest_fname[ppd_nest_level] = gu_strdup(ppd_file_name);	/* !!! */

	/* call the lexer to process the file */
	yylex();

	/* Now, close the outermost file, but DON'T deallocate the name: */
	fclose(yyin);

	#else
	{
	char *line, *p;

	/* open the PPD file */
	if(ppd_open(ppd_file_name, NULL) != EXIT_OK)
		fatal(EXIT_PRNERR_NORETRY, "%s(): can't open \"%s\", errno=%d (%s)", function, ppd_file_name, errno, gu_strerror(errno));

	while((line = ppd_readline()))
		{
		if(instring)
			{
			char *p2;
			if((p2 = strchr(line, '"')))
				*p2 = '\0';
			if(*line)
				ppd_callback_string_line(line);
			if(p2)
				ppd_callback_end_string();
			}
		else if(line[0] == '*')
			{
			switch(line[1])
				{
				case '%':				/* comment */
					continue;
					break;
				case 'c':
					if((p = lmatchp(line, "*cupsFilter:")))
						{
						if(*p == '"')
							{
							p++;
							p[strcspn(p, "\"")] = '\0';
							ppd_callback_cups_filter(p);
							}
						continue;
						}
					break;
				case 'D':
					if((p = lmatchp(line, "*DefaultOutputOrder:")))
						{
						DODEBUG_PPD_DETAILED(("%s(): *DefaultOutputOrder: %s", function, p));
						if(strcmp(p, "Normal") == 0)
							printer.OutputOrder = 1;
						else if(strcmp(p, "Reverse") == 0)
							printer.OutputOrder = -1;
						else
							error("Unrecognized \"*OutputOrder:\" in PPD file: \"%s\"", p);
						continue;
						}
					else if((p = lmatchp(line, "*DefaultResolution:")))
						{
						ppd_callback_resolution(p);
						continue;
						}
					break;
				case 'E':
					if((p = lmatchp(line, "*Extensions:")))
						{
						char *p2;
						while((p2 = gu_strsep(&p, " \t")))
							{
							DODEBUG_PPD_DETAILED(("%s(): *Extensions: %s", function, p2));
							if(strcmp(p2, "DPS") == 0)
								Features.Extensions |= EXTENSION_DPS;
							else if(strcmp(p2, "CMYK") == 0)
								Features.Extensions |= EXTENSION_CMYK;
							else if(strcmp(p2, "Composite") == 0)
								Features.Extensions |= EXTENSION_Composite;
							else if(strcmp(p2, "FileSystem") == 0)
								Features.Extensions |= EXTENSION_FileSystem;
							else
								error("Unrecognized \"*Extensions:\" in PPD file: \"%s\"", p2);
							}
						continue;
						}
					break;
				case 'F':
					if((p = lmatchp(line, "*FaxSupport:")))
						{
						char *p2;
						while((p2 = gu_strsep(&p, " \t")))
							{
							DODEBUG_PPD_DETAILED(("%s(): *FaxSupport: %s", function, p2));
							if(strcmp(p2, "Base") == 0)
								Features.FaxSupport |= FAXSUPPORT_Base;
							else
								error("Unrecognized \"*FaxSupport:\" in PPD file: \"%s\"", p2);
							}
						continue;
						}
					else if((p = lmatchp(line, "*FileSystem:")))
						{
						DODEBUG_PPD_DETAILED(("%s(): *FileSystem: %s", function, p));
						if(strcmp(p, "True") == 0)
							Features.FileSystem = TRUE;
						else if(strcmp(p, "False") == 0)
							Features.FileSystem = FALSE;
						else
							error("Unrecognized \"*FileSystem:\" in PPD file: \"%s\"", p);
						continue;
						}
					if((p = lmatchsp(line, "*Font")))
						{
						p[strcspn(p, ":")] = '\0';
						ppd_callback_add_font(p);
						continue;
						}
					break;
				case 'I':
					if((p = lmatchsp(line, "*ImageableArea")))
						{
						p += strcspn(p, ":");
						if(*p == ':' && strlen(p) > 1)
							{
							p++;
							*p++ = '\0';
							ppd_callback_papersize_moveto(line);
							ppd_callback_new_string(line);
							p += strspn(p, " \t");
							p += strspn(p, "\"");
							p[strcspn(p, "\"")] = '\0';
							DODEBUG_PPD_DETAILED(("%s(): %s \"%s\"", function, line, p));
							sscanf(p, "%lf %lf %lf %lf",
								&papersize[papersizex].lm,
								&papersize[papersizex].tm,
								&papersize[papersizex].rm,
								&papersize[papersizex].bm
								);
							ppd_callback_string_line(p);
							ppd_callback_end_string();
							}
						continue;
						}
					break;
				case 'L':
					if((p = lmatchp(line, "*LanguageLevel:")))
						{
						if(*p == '"')
							{
							p++;
							p[strcspn(p, "\"")] = '\0';
							DODEBUG_PPD_DETAILED(("%s(): *LanguageLevel: %s", function, p));
							Features.LanguageLevel = atoi(p);
							line[strcspn(line, ":")] = '\0';
							ppd_callback_new_string(line);
							ppd_callback_string_line(p);
							ppd_callback_end_string();
							}
						else
							{
							error("Invalid \"*LanguageLevel:\" in PPD file");
							}
						continue;
						}
					break;
				case 'M':
					if(lmatch(line, "*Message:"))
						continue;
					break;
				case 'O':
					if((p = lmatchp(line, "*OrderDependency:")))
						{
						ppd_callback_order_dependency(p);
						continue;
						}
					break;
				case 'P':
					if((p = lmatchsp(line, "*PaperDimension")))
						{
						p += strcspn(p, ":");
						if(*p == ':' && strlen(p) > 1)
							{
							p++;
							*p++ = '\0';
							ppd_callback_papersize_moveto(line);
							ppd_callback_new_string(line);
							p += strspn(p, " \t");
							p += strspn(p, "\"");
							p[strcspn(p, "\"")] = '\0';
							DODEBUG_PPD_DETAILED(("%s(): %s \"%s\"", function, line, p));
							sscanf(p, "%lf %lf",
								&papersize[papersizex].width,
								&papersize[papersizex].height
								);
							ppd_callback_string_line(p);
							ppd_callback_end_string();
							}
						continue;
						}
					else if((p = lmatchp(line, "*Protocols:")))
						{
						char *p2;
						while((p2 = gu_strsep(&p, " \t")))
							{
							DODEBUG_PPD_DETAILED(("%s(): *Protocols: %s", function, p2));
							if(strcmp(p2, "TBCP") == 0)
								printer.prot.TBCP = TRUE;
							else if(strcmp(p2, "PJL") == 0)
								printer.prot.PJL = TRUE;
							else
								error("Unrecognized \"*Protocols:\" in PPD file: \"%s\"", p2);
							}
						continue;
						}
					else if(lmatch(line, "*PrinterError:"))
						continue;
					break;
					break;
				case 'p':
					if((p = lmatchp(line, "*pprRIP:")))
						{
						ppd_callback_rip(p);
						continue;
						}
					break;
				case 'S':
					if(lmatch(line, "*Source:"))
						continue;
					else if(lmatch(line, "*Status:"))
						continue;
					break;
				case 'T':
					if((p = lmatchp(line, "*TTRasterizer:")))
						{
						char *p2;
						while((p2 = gu_strsep(&p, " \t")))
							{
							DODEBUG_PPD_DETAILED(("%s(): *TTRasterizer: %s", function, p2));
							if(strcmp(p2, "None") == 0)
								Features.TTRasterizer = TT_NONE;
							else if(strcmp(p2, "Accept68K") == 0)
								Features.TTRasterizer = TT_ACCEPT68K;
							else if(strcmp(p2, "Type42") == 0)
								Features.TTRasterizer = TT_TYPE42;
							else
								error("Unrecognized \"*TTRasterozer:\" in PPD file: \"%s\"", p2);
							}
						continue;
						}
					break;
				default:
					break;
				}

			/* Wasn't caught above */
			if((p = strchr(line, ':')) && strlen(p) > 1)
				{
				p++;
				*p++ = '\0';
				p += strspn(p, " \t");
				if(*p == '"')
					{
					char *p2;
					p++;
					ppd_callback_new_string(line);
					if((p2 = strchr(p, '"')))
						*p2 = '\0';
					ppd_callback_string_line(p);
					if(p2)
						ppd_callback_end_string();
					}
				}


			}
		}
	}
	#endif

	/* Free the scratch spaces: */
	gu_free(ppdname);
	gu_free(ppdtext);

	/* If we still haven't been told to use a RIP, see if we saw a "*cupsFilter:" 
	   line that can help us.
	   */
	DODEBUG_PPD(("read_PPD_file(): printer.RIP.name=\"%s\", cups_filter=\"%s\", default_resolution=%d",
		printer.RIP.name ? printer.RIP.name : "",
		cups_filter ? cups_filter : "",
		default_resolution));
	if(!printer.RIP.name && cups_filter && default_resolution > 0)
		{
		printer.RIP.name = "ppr-gs";
		printer.RIP.output_language = "pcl";
		gu_asprintf(&printer.RIP.options_storage, "cups=%s -r%d", cups_filter, default_resolution);
		}

	if(cups_filter)
		gu_free(cups_filter);
		
	} /* read_PPD_file() */

/*=========================================================================
** Routines for Feature Inclusion
=========================================================================*/

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
	struct PPDSTR *s;					/* pointer to PPD database entry */
	int len;							/* used in comparison */
	char *string = (char*)NULL;			/* value to return */

	DODEBUG_PPD(("find_feature(\"%s\", \"%s\")", featuretype, option ? option : ""));

	s = ppdstr[hash2(featuretype, option, PPD_TABSIZE)];

	while(s)
		{
		if( (strncmp(s->name,featuretype,len=strlen(featuretype))==0)
			&& ( ( option==(char*)NULL && s->name[len]=='\0' )
				|| ( s->name[len]==' ' && strcmp(&s->name[len+1],option)==0 ) )
			) /* <-- notice parenthesis */
			{
			string = s->value;					/* use the value */
			break;
			}
		else									/* if no match */
			{									/* if chain continues, */
			s = s->next;						/* jump to next link */
			}
		}

	return string;
	} /* end of find_feature() */

/*
** This is called by both include_feature() and begin_feature().  It
** determines if we have to remove or change this feature.  A feature
** is changed by replacing featuretype or option using the 
** pointers provided.  To remove a feature, set both to NULL.
*/
static void feature_change(const char **featuretype, const char **option)
	{
	if(strip_binselects)
		{
		if(strcmp(*featuretype, "*InputSlot") == 0)
			{
			*featuretype = *option = NULL;
			return;
			}
		if(strcmp(*featuretype, "*TraySwitch") == 0)
			{
			*featuretype = *option = NULL;
			return;
			}
		if(strcmp(*featuretype, "*PageSize") == 0)
			{
			*featuretype = "*PageRegion";
			return;
			}
		}
	if(strip_signature)
		{
		if(strcmp(*featuretype, "*Signature") == 0)
			{
			*featuretype = *option = NULL;
			return;
			}
		if(strcmp(*featuretype, "*Booklet") == 0)
			{
			*featuretype = *option = NULL;
			return;
			}
		}
	if(copies_auto_collate)
		{
		if(strcmp(*featuretype, "*Collate") == 0)
			{
			if(copies_auto_collate != -1)
				{
				*option = copies_auto_collate > 0 ? "True" : "False";
				copies_auto_collate = -1;
				}
			return;
			}
		}
	} /* end of feature_change() */

/*
** Insert the PostScript code for a given PPD-described feature, if the PPD
** file contains such a description of it.  If not, insert an
** "%%IncludeFeature:" comment so that a spooler furthur down the line can
** insert it if it has it.
**
** The parameter featuretype[] might be "*Duplex" and option[] might
** be "DuplexNoTumble".  It is possible for option[] to be NULL.
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
	   (I think that a non-NULL but empty option is not the right way to
	   represent a missing option.)
	   */
	if(option && !option[0])
		fatal(EXIT_PRNERR_NORETRY, "%s(): assertion failed (featuretype[]=\"%s\" option[]=\"\")", function, featuretype);

	/* Here we call feature_change() so it can tell us if we have to 
	   remove or change this feature.  (One reasion why we might have
	   to do this if we are changing the bin selection.
	   */
	{
	const char *new_featuretype = featuretype, *new_option = option;
	feature_change(&new_featuretype, &new_option);
	if(new_featuretype != featuretype || new_option != option)
		{
		#ifdef KEEP_OLD_CODE
		printer_printf("%% %%%%IncludeFeature: %s%s%s\n", featuretype, option ? " " : "", option ? option : "");
		#endif
		if(!new_featuretype)
			return;
		featuretype = new_featuretype;
		option = new_option;
		}
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
		else									/* no option keyword, convert QuotedValue */
			printer_puts_QuotedValue(string);	/* to binary and write it to the printer. */

		/* and flag its end */
		printer_puts("\n%%EndFeature\n");
		}

	/*
	** If we are using N-Up and it is a *PageRegion feature, guess at the 
	** correct code.  We do this because N-Up prints virtual pages and the
	** the N-Up machinery may be able to interpret many commands that the 
	** printer can't.
	*/
	else if(job.N_Up.N != 1 && strcmp(featuretype, "*PageRegion") == 0 && option)
		{
		const char *ptr;
		printer_printf("%%%%BeginFeature: *PageRegion %s\n", option);
		for(ptr=option; *ptr; ptr++)
			printer_putc(tolower(*ptr));
		printer_puts(option);
		printer_puts("\n%%EndFeature\n");
		}

	/*
	** As a last ditch effort, just insert an "%%IncludeFeature:" comment.
	*/
	else
		{
		feature_warning(_("The PPD file doesn't contain code for the printer feature\n"
				"called \"%s %s\", skipping it."), featuretype, option ? option : "");

		printer_printf("%%%%IncludeFeature: %s%s%s\n", featuretype, option ? " " : "", option ? option : "");
		}

	} /* end of include_feature() */

/*
** We call this function when we have "%%BeginFeature: ..." in line[].
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
**
** This function does not leave anything useful in line[].
*/
void begin_feature(const char featuretype[], const char option[], FILE *infile)
	{
	const char *function = "begin_feature";
	const char *string = NULL;
	gu_boolean keep = FALSE;

	DODEBUG_PPD(("%s()", function));

	/* Ignore blank feature lines. */
	if(!featuretype)
		return;

	/* Here we call feature_change() so it can tell us if we have to remove or change this feature. */
	{
	const char *new_featuretype = featuretype, *new_option = option;
	feature_change(&new_featuretype, &new_option);
	if(new_featuretype != featuretype || new_option != option)
		{
		#ifdef KEEP_OLD_CODE
		printer_printf("%% %%%%BeginFeature: %s%s%s\n", featuretype, option ? " " : "", option ? option : "");
		#endif
		featuretype = new_featuretype;
		option = new_option;
		}
	}

	/* If feature isn't being deleted, send the start comment. */
	if(featuretype)
		{
		if(option)
			printer_printf("%%%%BeginFeature: %s %s\n", featuretype, option);
		else
			printer_printf("%%%%BeginFeature: %s\n", featuretype);
		}

	/* If the feature isn't going to be deleted, try to find its PPD code. */
	if(featuretype)
		{
		if((string = find_feature(featuretype, option)))
			{
			}
		else if(job.opts.keep_badfeatures)
			{
			feature_warning(_("The PPD file doesn't contain code for the printer feature\n"
						"called \"%s %s\", retaining old code."), featuretype, option ? option : "");

			printer_puts("% PPR retained the following feature code despite fact that no such\n"
							 "% feature is listed in the PPD file.  Use ppr's -K switch to change\n"
							 "% this behavior.\n");
			keep = TRUE;
			}
		else
			{
			feature_warning(_("The PPD file doesn't contain code for the printer feature\n"
				"called \"%s %s\", removing existing code."), featuretype, option ? option : "");

			printer_putline("% The code for this printer feature was removed by PPR because\n"
							"% there is no such feature listed in the PPD file.  Use ppr's -K\n"
							"% switch to change this behavior.");
			}
		}

	/* Chug through the old feature lines. */
	while(TRUE)
		{
		if(dgetline(infile) == (char*)NULL)
			{
			give_reason("defective feature invokation");
			fatal(EXIT_JOBERR, "%s(): unterminated feature code", function);
			}

		if(strcmp(line, "%%EndFeature") == 0)
			break;

		if(keep)
			{
			printer_putline(line);
			}
		#ifdef KEEP_OLD_CODE
		else
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

	if(featuretype)
		printer_putline("%%EndFeature");
	else
		printer_putline("% %%EndFeature");
	} /* end of begin_feature() */

/*
** PostScript stopped context.
**
** These routines emmit PostScript code which surrounds any feature
** invokation that we insert.  This bracketing code catches errors
** so that if, say, the duplex command failes, the job will still
** print.
**
** We call these functions to bracket additional feature invokations
** which we add.
*/
void begin_stopped(void)
	{
	printer_putline("[ { %PPR");
	} /* end of begin_stopped() */

void end_stopped(const char *feature, const char *option)
	{
	printer_printf("} stopped {(PPD error: %s %s failed\\n)print} if cleartomark %%PPR\n",
		feature,
		option ? option : ""
		);
	} /* end of end_stopped() */

/*
** Insert one feature for each "Feature:" line read from qstream.  These lines
** represent the -F switches from the ppr command line.
**
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
	const char function[] = "insert_features";
	static char *duplex_code = (char*)NULL;
	char *line = NULL;
	int line_space = 80;
	char *feature_type, *feature_option;
	int len;
	gu_boolean set_strip_binselects = FALSE;

	if(set < 1 || set > 2)
		fatal(EXIT_PRNERR_NORETRY, "%s(): assertion failed (set=%d)", function, set);

	if(set==2)			/* If this is set 2, insert the code for the */
		{				/* remembered duplex feature. */
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

	/* Something is wrong if this isn't set 2 and set 1 has already been done. */
	if(duplex_code)
		fatal(EXIT_PRNERR_NORETRY, "%s(): assertion failed (duplex_code[]=\"%s\")", function, duplex_code);

	while((line = gu_getline(line, &line_space, qstream)))
		{
		feature_option = NULL;

		if(strcmp(line, "EndSetups") == 0)
			break;

		/*
		** We are looking for lines like these:
		**
		**		Feature: *Duplex DuplexNoTumble
		**		Feature: *Reset
		*/
		if((feature_type = lmatchp(line, "Feature:")))
			{
			len = strcspn(feature_type, " \t");

			/* If the line doesn't end with the end of the feature_type, */
			if(feature_type[len])
				{
				feature_option = &feature_type[len + 1];		/* just beyond space */
				feature_type[len] = '\0';
				}

			/*
			** If the code to be inserted is a duplex feature command,
			** remember it for insertion during pass 2,
			*/
			if(strcmp(feature_type, "*Duplex") == 0 && feature_option)
				{
				duplex_code = gu_strdup(feature_option);
				}

			/*
			** All other features get inserted right away.
			*/
			else
				{
				begin_stopped();
				include_feature(feature_type, feature_option);
				end_stopped(feature_type, feature_option);

				/*
				** New as of 1.30:	Explicitly selecting the input slot causes
				** old bin select code to be stript out and PageSize code
				** to be turned into PageRegion code.
				*/
				if(strcmp(feature_type, "*InputSlot") == 0)
					set_strip_binselects = TRUE;
				}
			}

		} /* end of while loop which itemizes features to insert */

	if(line)
		gu_free(line);

	if(set_strip_binselects)
		strip_binselects = TRUE;

	} /* end of insert_features() */

/*======================================================================
** ppd_find_font() is called from pprdrv_res.c and pprdrv_capable.c.
** ppd_find_font() returns non-zero if it can't find the font
** in the list from the PPD file.
======================================================================*/

gu_boolean ppd_font_present(const char fontname[])
	{
	int h;
	struct PPDFONT *p;				/* pointer to font structure */

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

/*======================================================================
** Printer collate support
======================================================================*/

gu_boolean printer_can_collate(void)
	{
	/* Disable until we can parse UIContraints */
	return FALSE;

	if(find_feature("*Collate", "True"))
		return TRUE;
	else
		return FALSE;
	}

void set_collate(gu_boolean collate)
	{
	if(collate)
		{
		begin_stopped();
		include_feature("*Collate", "True");
		end_stopped("*Collate", "True");
		}
	}

/* end of file */

