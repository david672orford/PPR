/*
** mouse:~ppr/src/ppr/ppr_features.c
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
** Last modified 14 August 2002.
*/

#include "before_system.h"
#include <string.h>
#include <ctype.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "ppr.h"
#include "ppr_exits.h"

/*
** This is for the --features option.
*/
int option_features(const char destnode[], const char destname[])
    {
    const char *ppdfile;
    char *line;
    char *p;
    int len;
    char *ui = NULL;
    char *ui_default = NULL;
    char *group = NULL;
    
    if(!(ppdfile = dest_ppdfile(destnode, destname)))
    	{
	fprintf(stderr, "%s: don't know which PPD file\n", myname);
	return PPREXIT_OTHERERR;
    	}

    if(ppd_open(ppdfile, stderr) == -1)
    	return PPREXIT_OTHERERR;

    while((line = ppd_readline()))
	{
	/*printf("%s", line);*/
	if((p = lmatchp(line, "*OpenGroup:")))
	    {
	    len = strcspn(p, "/ \t");
	    group = gu_strndup(p, len);
	    /*printf("group: %s\n", group);*/
	    continue;
	    }
	if(group && lmatch(line, "*CloseGroup:"))
	    {
	    gu_free(group);
	    group = NULL;
	    continue;
	    }
	if(!ui && (p = lmatchp(line, "*OpenUI")) && *p == '*' && (!group || strcmp(group, "InstallableOptions")))
	    {
	    p++;
	    len = strcspn(p, "/:");
	    ui = gu_strndup(p, len);
	    p += len;
	    if(*p == '/')		/* if translation string exists, */
		{
		p++;
	    	printf("%.*s\n", (int)strcspn(p, ":"), p);
	    	}
	    else			/* if no translation string, */
	    	{
	    	printf("%s\n", ui);
	    	}
	    continue;
	    }
	if(ui && !ui_default && lmatch(line, "*Default") && strncmp((p=line+8), ui, strlen(ui)) == 0 && p[strlen(ui)] == ':')
	    {
	    p += strlen(ui);
	    p++;
	    p += strspn(p, " \t");
	    ui_default = gu_strndup(p, strcspn(p, " \n"));
	    /*printf("default: %s\n", ui_default);*/
	    } 
	if(ui && line[0] == '*' && strncmp(line+1, ui, strlen(ui)) == 0 && isspace(line[1+strlen(ui)]))
	    {
	    char *translation;
	    p = line + 1 + strlen(ui);		/* Set p to point after "*Feature ". */
	    p += strspn(p, " \t");
	    len = strcspn(p, "/:");		/* Get length to end of feature option name. */

	    if(p[len] == '/')			/* If there is a translation string, */
	    	{
	    	translation = p + len + 1;
	    	translation[strcspn(translation, ":")] = '\0';
	    	}
	    else
	    	{
	    	translation = p;
	    	}
	    p[len] = '\0';

	    printf(" %s%-28s  --feature %s=%s\n",
		(ui_default && strcmp(p, ui_default) == 0) ? "-->" : "   ",
		translation,
		ui,
		p);
	    continue;
	    }
	if(ui && lmatch(line, "*CloseUI:"))
	    {
	    gu_free(ui);
	    ui = NULL;
	    if(ui_default)
	    	gu_free(ui_default);
	    ui_default = NULL;
	    printf("\n");
	    continue;
	    }

	}

    if(ui)
	gu_free(ui);
    if(ui_default)
    	gu_free(ui_default);
    if(group)
    	gu_free(group);

    return PPREXIT_OK;
    }

/* end of file */