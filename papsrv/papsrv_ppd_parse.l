%{
/*
** mouse:~ppr/src/papsrv/papsrv_ppd_parse.l
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
** Last modified 1 December 2000.
*/

/*
** This code is the lexer which the AppleTalk PAP server uses
** to parse an Adobe PPD file.
*/

#include "before_system.h"
#include <stdlib.h>		/* for exit() and atoi() */
#ifdef FLEX_SCANNER
#include <unistd.h>		/* for read() (flex uses it) */
#include <sys/uio.h>		/* for read() (flex uses it) */
#endif
#include <string.h>
#include <errno.h>
#include "gu.h"
#include "global_defines.h"
#include "papsrv.h"		/* for printer structure */

extern struct ADV *printer;
void add_font(char *fontname);

/* This is where we save the original input file while we
   are reading from an include file. */
extern char *ppd_nest_fname[MAX_PPD_NEST];
extern int ppd_nest_level;

/* This is where we save the original input file while we
   are reading from an include file.  The array size is one less than
   MAX_PPD_NEST because the state of the last file is never saved. */
#ifdef FLEX_SCANNER
YY_BUFFER_STATE ppd_nest_buf[MAX_PPD_NEST - 1];
#else
FILE *ppd_nest_f[MAX_PPD_NEST - 1];
#endif

/* Some Lex implementations define yywrap as a macro,
   if that has been done, get rid of it. */
#undef yywrap

/* Here is a yywrap() which switches back to the original file
   when the include file ends. */
#ifdef __cplusplus
extern "C" int yywrap(void);
#endif
int yywrap(void)
    {
    if(ppd_nest_level > 0)
	{
	DODEBUG_PPD(("end of include file"));

	gu_free(ppd_nest_fname[ppd_nest_level]);	/* don't need file name anymove */
	ppd_nest_level--;

	#ifdef FLEX_SCANNER
	yy_delete_buffer(YY_CURRENT_BUFFER);
	yy_switch_to_buffer(ppd_nest_buf[ppd_nest_level]);
	#else
	fclose(yyin);				/* close include file */
	yyin = ppd_nest_f[ppd_nest_level];	/* restore current input file */
	#endif

	return 0;			/* Tell parser that we have opened another file. */
	}				/* (Which is sort of what we have done.) */

    return 1;
    }

%}

%s FAX
%s LANGLEV
%s PSVER
%s PRODUCT
%s FREEVM
%s VMOPT
%s VMOPT2
%s VMOPT3
%s RES
%s FONT
%s FONTJUNK
%s TTRAS
%s INCLUDE

%%

<INITIAL>^\*FaxSupport:[ \t]* { BEGIN FAX; }
<FAX>[^ \t\n]+		{ if(printer->FaxSupport == (char*)NULL) /* 1st one only */
				printer->FaxSupport = gu_strdup(yytext); }
<FAX>\n                 { BEGIN INITIAL; }

<INITIAL>^\*LanguageLevel:[ \t]*["] { BEGIN LANGLEV; }
<LANGLEV>[0-9]+         { if(printer->LanguageLevel == 0)	/* 1st one only */
				printer->LanguageLevel = atoi(yytext); }
<LANGLEV>["]            { BEGIN INITIAL; }

<INITIAL>^\*Product:[ \t]*["] { BEGIN PRODUCT; }
<PRODUCT>[^"]+          { if(printer->Product == (char*)NULL) /* 1st one */
			    printer->Product = gu_strdup(yytext); }
<PRODUCT>["]            { BEGIN INITIAL; }

<INITIAL>^\*PSVersion:[ \t]*["] { BEGIN PSVER; }
<PSVER>[^"]+            { if(printer->PSVersion == (char*)NULL) /* 1st one */
			    printer->PSVersion = gu_strdup(yytext); }
<PSVER>["]              { BEGIN INITIAL; }

<INITIAL>^\*FreeVM:[ \t]*["] { BEGIN FREEVM; }
<FREEVM>[0-9]+          { if(printer->FreeVM == 0)		/* 1st one only */
			    printer->FreeVM = atoi(yytext); }
<FREEVM>["]             { BEGIN INITIAL; }

<INITIAL>^\*VMOption[ \t]+ { BEGIN VMOPT; }
<VMOPT>[^ \t\n/:]+	{
			/* If there was a PPDOpt: line in the printer
			   configuration file for *InstalledMemory and it
			   matches this *VMOption line, */
			if( printer->InstalledMemory != (char*)NULL && strcmp(printer->InstalledMemory, yytext) == 0)
				{ BEGIN VMOPT2; }
			else
				{ BEGIN INITIAL; }
			}
<VMOPT2>[^"\n]+["]	{ BEGIN VMOPT3; }
<VMOPT3>[0-9]+		{ if( printer->VMOptionFreeVM == 0 )
				printer->VMOptionFreeVM = atoi(yytext); }

<INITIAL>^\*DefaultResolution:[ \t]* { BEGIN RES; }
<RES>[^ \t\n]+		{ if(printer->Resolution==(char*)NULL)
				printer->Resolution=gu_strdup(yytext); }

<INITIAL>^\*Font[ \t\n]+	{ BEGIN FONT; }
<FONT>[^:\n]+			{ BEGIN FONTJUNK;
				add_font(yytext); }
<FONTJUNK>.*			{ /* discard it */ }

<INITIAL>^\*TTRasterizer:[ \t]* { BEGIN TTRAS; }
<TTRAS>[^ \t\n]+	{ if(printer->TTRasterizer == (char*)NULL)
			    printer->TTRasterizer = gu_strdup(yytext); }

<INITIAL>^\*Include:[ \t]*["]	{ BEGIN INCLUDE; }
<INCLUDE>[^"\n]*		{
	char *ifname;		/* include file name */

	if(++ppd_nest_level >= MAX_PPD_NEST)
	    fatal(1, "Too many levels of \"*Include:\" in PPD file");

	if(yytext[0] == '/')
	    {
	    ifname = gu_strdup(yytext);
	    }
	else
	    {
	    char *pathend;
	    int len;
	    size_t space_needed;

	    if((pathend = strrchr(ppd_nest_fname[ppd_nest_level-1], '/')) == (char*)NULL)
		fatal(1, "papsrv_ppd_parse.l: internal error: no /");

	    len = pathend - ppd_nest_fname[ppd_nest_level-1];

	    space_needed = (len + 1 + strlen(yytext) + 1);
	    ifname = (char*)gu_alloc(space_needed, sizeof(char));
	    snprintf(ifname, space_needed, "%.*s/%s", len, ppd_nest_fname[ppd_nest_level-1], yytext);
	    }

	ppd_nest_fname[ppd_nest_level] = ifname;

	/* Attempt to save the scanner's place in the file. */
	#ifdef FLEX_SCANNER
	ppd_nest_buf[ppd_nest_level - 1] = YY_CURRENT_BUFFER;
	#else
	ppd_nest_f[ppd_nest_level - 1] = yyin;
	#endif

	DODEBUG_PPD(("opening include file \"%s\"", ifname));
	if((yyin = fopen(ifname, "r")) == (FILE*)NULL)
	    fatal(1, "PPD include file \"%s\" can't be opened, errno=%d (%s)", ifname, errno, strerror(errno));

	#ifdef FLEX_SCANNER
	yy_switch_to_buffer(yy_create_buffer(yyin, YY_BUF_SIZE));
	#endif

	/* We must reset the state or it will look for an *Include: argument
	   in the include file.  We will rely on the default rule to swallow
	   the quote and newline. */
	BEGIN INITIAL;
	}

.|\n         { BEGIN INITIAL; /* discard unmatched stuff */ }

%%
