%{
/*
** mouse:~ppr/src/pprdrv/pprdrv_ppd_parse.l
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
** Last modified 30 March 2001.
*/

/*
** This code is the lexer which pprdrv uses to parse an Adobe PPD file.
**
** It finds certain lines it is interested in, particularly those
** which give feature code and loads them into a hashed data
** structure so that they may be used when printing the document.
*/

#include "before_system.h"
#include <stdlib.h>		/* for exit() and atoi() */
#ifdef FLEX_SCANNER
#include <unistd.h>		/* for read() (flex uses it) */
#include <sys/uio.h>		/* for read() (flex uses it) */
#endif
#include <errno.h>
#include <string.h>
#include "gu.h"
#include "global_defines.h"	/* for feature values */
#include "pprdrv.h"		/* for feature structure */
#include "interface.h"		/* for exit codes */

/* This is where we save the original input file while we
   are reading from an include file.  The array size is
   one less than MAX_PPD_NEST because the state of the
   last file is never saved. */
#ifdef FLEX_SCANNER
YY_BUFFER_STATE ppd_nest_buf[MAX_PPD_NEST - 1];
#else
FILE *ppd_nest_f[MAX_PPD_NEST - 1];
#endif

/* Some Lex implementations define yywrap as a macro,
   if that has been done, get rid of it. */
#undef yywrap

/* Here is a yywrap() which switches back to the
   original file when the include file ends. */
#ifdef __cplusplus
extern "C" int yywrap(void);
#endif
int yywrap(void)
    {
    if(ppd_nest_level > 0)
	{
	DODEBUG_PPD(("end of include file"));

	gu_free(ppd_nest_fname[ppd_nest_level]);		/* don't need file name anymove */
	ppd_nest_level--;

	#ifdef FLEX_SCANNER
	yy_delete_buffer(YY_CURRENT_BUFFER);
	yy_switch_to_buffer(ppd_nest_buf[ppd_nest_level]);
	#else
	fclose(yyin);					/* close include file */
	yyin = ppd_nest_f[ppd_nest_level];		/* restore current input file */
	#endif

	return 0;			/* Tell parser that we have opened another file. */
	}				/* (Which is sort of what we have done.) */

    return 1;
    }

%}

%s FONT
%s FONTJUNK
%s JUNKSTR
%s INSTR
%s EXT
%s FAX
%s LANGLEV
%s PAPDIM
%s IMGAREA
%s INC
%s ORD1
%s ORD2
%s ORD3
%s ORD4
%s PRTCOL

%p 3500

%%

<INITIAL>^\*Extensions:   { BEGIN EXT; }
<EXT>[ \t]+               { /* discard spaces and tabs */ }
<EXT>DPS/[ \t\n]          { Features.Extensions|=EXTENSION_DPS; }
<EXT>CMYK/[ \t\n]         { Features.Extensions|=EXTENSION_CMYK; }
<EXT>Composite/[ \t\n]    { Features.Extensions|=EXTENSION_Composite; }
<EXT>FileSystem/[ \t\n]   { Features.Extensions|=EXTENSION_FileSystem; }
<EXT>[^ \t\n]+/[ \t\n]    { error("Invalid \"*Extensions:\" in PPD file: \"%s\"",
			    yytext); }
<EXT>\n                   { BEGIN INITIAL; }

<INITIAL>^\*FaxSupport: { BEGIN FAX; }
<FAX>[ \t]+             { /* discard spaces and tabs */ }
<FAX>Base/[ \t\n]       { Features.FaxSupport|=FAXSUPPORT_Base; }
<FAX>[^ \t\n]+/[ \t\n]  { error("Invalid \"*FaxSupport:\" in PPD file: \"%s\"",
			    yytext); }
<FAX>\n                 { BEGIN INITIAL; }

<INITIAL>^\*FileSystem:[ \t]+True[ \t]*\n {
	Features.FileSystem=TRUE; }
<INITIAL>^\*FileSystem:[ \t]+False[ \t]*\n {
	Features.FileSystem=FALSE; }

<INITIAL>^\*TTRasterizer:[ \t]+None[ \t]*\n {
	Features.TTRasterizer=TT_NONE; }
<INITIAL>^\*TTRasterizer:[ \t]+Accept68K[ \t]*\n {
	Features.TTRasterizer=TT_ACCEPT68K; }
<INITIAL>^\*TTRasterizer:[ \t]+Type42[ \t]*\n {
	Features.TTRasterizer=TT_TYPE42; }

<INITIAL>^\*LanguageLevel:[ \t]*["] {
			BEGIN LANGLEV; }
<LANGLEV>[0-9]+         { Features.LanguageLevel = atoi(yytext); }
<LANGLEV>["]            { BEGIN INITIAL; }

<INITIAL>^\*Font[ \t]+	{ BEGIN FONT; }
<FONT>[^:\n]+		{
			add_font(yytext);
			BEGIN FONTJUNK;
			}
<FONTJUNK>.*		{ /* discard it */ }

<INITIAL>^\*DefaultOutputOrder:[ \t]+Normal[ \t]*\n {
	if(printer.OutputOrder == 0)
	    printer.OutputOrder = 1;
	}
<INITIAL>^\*DefaultOutputOrder:[ \t]+Reverse[ \t]*\n {
	if(printer.OutputOrder == 0)
	    printer.OutputOrder = -1;
	}

<INITIAL>^\*Message:[ \t]+["]		{ BEGIN JUNKSTR; }
<INITIAL>^\*Status:[ \t]+["]		{ BEGIN JUNKSTR; }
<INITIAL>^\*Source:[ \t]+["]		{ BEGIN JUNKSTR; }
<INITIAL>^\*PrinterError:[ \t]+["]	{ BEGIN JUNKSTR; }
<INITIAL>^\*PCFileName:[ \t]+["]	{ BEGIN JUNKSTR; }
<JUNKSTR>[^"]|\n			{ /* discard data */ }
<JUNKSTR>["]				{ BEGIN INITIAL; }

<INITIAL>^\*PaperDimension[ \t]+[^\n:]+[:][ \t]+["] {
	papersize_moveto(yytext);
	new_string(yytext);
	BEGIN PAPDIM;
	}
<PAPDIM>[0-9]+[ ]+[0-9]+ {
	sscanf(yytext,"%lf %lf",
		&papersize[papersizex].width,
		&papersize[papersizex].height);
		string_line(yytext);
		}
<PAPDIM>["]	{ BEGIN INITIAL; }

<INITIAL>^\*ImageableArea[ \t]+[^\n:]+[:][ \t]+["] {
	papersize_moveto(yytext);
	new_string(yytext);
	BEGIN IMGAREA;
	}
<IMGAREA>[0-9]+[ ]+[0-9]+ { sscanf(yytext,"%lf %lf %lf %lf",
					&papersize[papersizex].lm,
					&papersize[papersizex].tm,
					&papersize[papersizex].rm,
					&papersize[papersizex].bm);
					string_line(yytext); }
<IMGAREA>["]			{ BEGIN INITIAL; }

<INITIAL>^\*Include:[ \t]+["]	{ BEGIN INC; }
<INC>[^"\n]* {
	char *ifname;		/* include file name */

	if(++ppd_nest_level >= MAX_PPD_NEST)
	    fatal(EXIT_PRNERR_NORETRY, "Too many levels of \"*Include:\" in PPD file");

	if(yytext[0] == '/')			/* absolute path */
	    {
	    ifname = gu_strdup(yytext);
	    }
	else
	    {
	    char *pathend;
	    int len;

	    if((pathend = strrchr(ppd_nest_fname[ppd_nest_level-1], '/')) == (char*)NULL)
		fatal(EXIT_PRNERR_NORETRY, "pprdrv: internal error: no /");

	    len = pathend - ppd_nest_fname[ppd_nest_level-1];

	    ifname = (char*)gu_alloc(len + 1 + strlen(yytext) + 1, sizeof(char));

	    sprintf(ifname, "%.*s/%s", len, ppd_nest_fname[ppd_nest_level-1], yytext);
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
	    fatal(EXIT_PRNERR_NORETRY, "Can't open PPD include file \"%s\", errno=%d (%s)", ifname, errno, strerror(errno));

	#ifdef FLEX_SCANNER
	yy_switch_to_buffer(yy_create_buffer(yyin, YY_BUF_SIZE));
	#endif

	/* We must reset the state or it will look for an *Include: argument
	   in the include file.  We will rely on the default rule to swallow
	   the quote and newline. */
	BEGIN INITIAL;
	}

<INITIAL>^\*[^ \t:\n]+:[ \t]+["] {
	BEGIN INSTR;
	new_string(yytext);
	}
<INITIAL>^\*[^? \t:\n]+[ \t]+[^ \t:\n]+(\/[^:\n]+)*:[ ]+["] {
	BEGIN INSTR;
	new_string(yytext);
	}
<INSTR>[^"\n]*	{ string_line(yytext); }
<INSTR>[\n]	{ /* discard the newline */ }
<INSTR>["]	{ BEGIN INITIAL; end_string(); }

<INITIAL>^\*OrderDependency:[ \t]+	{
					/* Gather info for sorting -F switches */
					BEGIN ORD1;
					}
<ORD1>[0-9]+				{ order_dependency_1(atoi(yytext));
					BEGIN ORD2; }
<ORD2>[ \t]+				{ }
<ORD2>ExitServer			{ order_dependency_2(ORD_EXITSERVER);
					BEGIN ORD3; }
<ORD2>Prolog				{ order_dependency_2(ORD_PROLOG);
					BEGIN ORD3; }
<ORD2>DocumentSetup			{ order_dependency_2(ORD_DOCUMENTSETUP);
					BEGIN ORD3; }
<ORD2>PageSetup				{ order_dependency_2(ORD_PAGESETUP);
					BEGIN ORD3; }
<ORD2>JCLSetup				{ order_dependency_2(ORD_JCLSETUP);
					BEGIN ORD3; }
<ORD2>AnySetup				{ order_dependency_2(ORD_ANYSETUP);
					BEGIN ORD3; }
<ORD3>[ \t]+				{ }
<ORD3>\*[^ \t\n]+			{ order_dependency_3(yytext);
					BEGIN ORD4; }
<ORD4>[ \t]+				{ }
<ORD4>[^ \t\n]+				{ order_dependency_4(yytext);
					BEGIN INITIAL; }
<ORD4>[\n]				{ order_dependency_4("");
					BEGIN INITIAL; }

<INITIAL>^\*Protocols:	{
	/* This is not perfect code since it doesn't test
	   for words, just substrings.  Note the absence
	   of [ \t\n]+ for consistency with code in ppad_printer.c */
			BEGIN PRTCOL;
			}
<PRTCOL>[ \t]+				{ }
<PRTCOL>TBCP				{ printer.prot.TBCP = TRUE; }
<PRTCOL>PJL				{ printer.prot.PJL = TRUE; }
<PRTCOL>[\n]				{ BEGIN INITIAL; }

.|\n         { BEGIN INITIAL; /* discard unmatched stuff */ }

%%

