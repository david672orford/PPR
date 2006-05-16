/*
** mouse:~ppr/src/ppr/ppr_features.c
** Copyright 1995--2006, Trinity College Computing Center.
** Written by David Chappell.
**
** This file is part of PPR.  You can redistribute it and modify it under the
** terms of the revised BSD licence (without the advertising clause) as
** described in the accompanying file LICENSE.txt.
**
** Last modified 10 May 2006.
*/

#include "config.h"
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
** This function is what the --features option calls.  It parses the PPD file
** and prints a list of the --feature options that will work with that
** PPD file.
*/
int option_features(const char destname[])
	{
	const char *ppdfile;
	void *ppdobj;
	char *line;
	char *p;
	int len;
	char *group = NULL;
	char *ui = NULL;
	char *ui_description = NULL;
	char *ui_default = NULL;
	char *ui_orderdependency = NULL;

	if(!(ppdfile = dest_ppdfile(destname)))
		{
		fprintf(stderr, _("%s: can't find destination %s or can't determine PPD file\n"), myname, destname);
		return PPREXIT_OTHERERR;
		}

	ppdobj = ppdobj_new(ppdfile);

	while((line = ppdobj_readline(ppdobj)))
		{
		/*printf("line[]: %s\n", line);*/
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
		/* If this is the start of a User Interface option cluster and we are not inside
		   a group or the group is not the InstallableOptions group, print the option
		   cluster name as a subheading.

		   An example:
		   		*OpenUI PageSize: PickOne
		   */
		if(!ui && (p = lmatchp(line, "*OpenUI")) && *p == '*' && (!group || strcmp(group, "InstallableOptions") != 0))
			{
			p++;
			len = strcspn(p, "/:");
			ui = gu_strndup(p, len);
			p += len;
			if(*p == '/')				/* if translation string exists, */
				{
				p++;
				ui_description = gu_strndup(p, strcspn(p, ":"));
				}
			else						/* if no translation string, */
				{
				ui_description = gu_strdup(ui);
				}
			continue;
			}
		if(ui && lmatch(line, "*CloseUI:"))
			{
			gu_free(ui);
			ui = NULL;
			if(ui_description)		/* if description wasn't used, */
				{
				gu_free(ui_description);
				ui_description = NULL;
				}
			else					/* if it was, leave a space before next feature */
				{
				printf("\n");
				}
			gu_free_if(ui_default);
			ui_default = NULL;
			gu_free_if(ui_orderdependency);
			ui_orderdependency = NULL;
			continue;
			}
		/* If we are in a User Interface option cluster and we haven't seen
		 * the default setting for this option yet, and this is a default
		 * setting, accept it.  The line might look like this:
		 *
		 * *DefaultDuplex: None
		 */
		if(ui && !ui_default && lmatch(line, "*Default") && strncmp((p=line+8), ui, strlen(ui)) == 0 && p[strlen(ui)] == ':')
			{
			p += strlen(ui);
			p++;
			p += strspn(p, " \t");
			ui_default = gu_strndup(p, strcspn(p, " \t"));
			/*printf("default: %s\n", ui_default);*/
			continue;
			}
		/* If we are in a User Interface option cluster and we haven't seen
		 * the order dependency line yet, and this is it, accept it.  The line
		 * might look like this:
		 *
		 * *OrderDependency: 20.0 AnySetup *Duplex
		 */
		if(ui && !ui_orderdependency && (p = lmatchp(line, "*OrderDependency:")))
			{
			p += strspn(p, "0123456789.-");
			p += strspn(p, " \t");
			ui_orderdependency = gu_strndup(p, strcspn(p, " \t"));
			continue;
			}
		/* If this is an option that matches the User Interface option
		   cluster, and it is suitable for the document setup section,
		   print it as a possible --feature switch.
		   */
		if(ui && line[0] == '*' && strncmp(line+1, ui, strlen(ui)) == 0 && isspace(line[1+strlen(ui)])
				&& (!ui_orderdependency || strcmp(ui_orderdependency, "AnySetup") == 0 || strcmp(ui_orderdependency, "DocumentSetup") == 0))
			{
			if(ui_description)
				{
				printf("%s\n", ui_description);
				gu_free(ui_description);
				ui_description = NULL;
				}
			{
			char *translation = NULL;
			p = line + 1 + strlen(ui);			/* Set p to point after "*Feature". */
			p += strspn(p, " \t");				/* eat up separator whitespace */

			len = strcspn(p, "/:");				/* Get length to end of feature option name. */
			if(p[len] == '/')					/* If there is a translation string, */
				{
				translation = p + len + 1;
				translation += strspn(translation, " \t");		/* eat spurious whitespace */
				translation[strcspn(translation, ":")] = '\0';	/* terminate translation */
				}
			p[len] = '\0';			/* terminate machine-readable */

			printf(" %s%-28s  --feature %s=%s\n",
				(ui_default && strcmp(p, ui_default) == 0) ? "-->" : "   ",		/* default mark */
				translation && strlen(translation) > 0 ? translation : p,		/* human-readable name */
				ui,																/* option */
				p																/* value */
				);										
			continue;
			}
			}
		}

	ppdobj_free(ppdobj);

	gu_free_if(group);
	gu_free_if(ui);
	gu_free_if(ui_description);
	gu_free_if(ui_default);
	gu_free_if(ui_orderdependency);

	return PPREXIT_OK;
	}

/* end of file */
