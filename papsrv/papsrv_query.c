/*
** mouse:~ppr/src/papsrv/papsrv_query.c
** Copyright 1995--1999, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is" without
** express or implied warranty.
**
** Last modified 5 August 1999.
*/

/*
** PAP server query answering routines.  These routines answer questions
** put to the spooler by the Macintosh client.  The information in the PPD file
** and the papsrv configuration file is used to answer these questions.
*/

#include "before_system.h"
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include "gu.h"
#include "global_defines.h"

#include "papsrv.h"

/*
** This module has two kinds of debugging in it.  The first kind
** is the type that is only compiled in if DEBUG_QUERY is defined.
** The second is always compiled in but is only turned on if
** the variable "query_trace" is non-zero.
**
** The variable "query_trace" can be cycled thru its meaningful
** values by sending SIGUSR1 to papsrv.
**
** Should we print minimal query debugging information?
** (This variable can have the values 0 (no debugging),
** 1 (trace queries, default replies and replies), and
** 2 (also print PostScript).)
*/
extern int query_trace;

/*
** This determines where we look to find things requested by resource queries.
** With LaserWriter 8.x we want a greedy font cache, so we will be saying
** something isn't present if it isn't in the cache, even if it is present
** elsewhere (we won't even look).
*/
static const enum RES_SEARCH font_search_list[] = { RES_SEARCH_CACHE, RES_SEARCH_END };
static const enum RES_SEARCH other_search_list[] = { RES_SEARCH_CACHE, RES_SEARCH_END };

/*
** A version of reply() which can be instructed print debugging
** information in the log file.
*/
void REPLY(int sesfd, char *ptr)
	{
	if(query_trace)
		debug("REPLY <-- %.*s", strcspn(ptr,"\n"), ptr);
	reply(sesfd,ptr);
	} /* end of REPLY() */

/*
** Before we answer a query, call this to eat up the PostScript
** code which a printer would interpret in order to answer
** the query.
**
** Return with the endquery line in line[].
*/
static void eat_query(int sesfd)
	{
	while(pap_getline(sesfd))
		{
		if(strncmp(line, "%%?End", 6) == 0)				/* If we have found the line that */
			{											/* marks the end of the query, */
			if(query_trace)
				{
				char *ptr;
				ptr = line;
				ptr += strcspn(ptr, " ");
				ptr += strspn(ptr, " ");
				debug("DEFAULT --> %s", ptr);
				}
			break;
			}

		if(query_trace > 1)										/* If level two query tracing */
			debug("PS --> %.*s",strcspn(line,"\n"),line);		/* is enabled, print PostScript code. */
		}
	} /* end of eat_query() */

/*
** If we can't answer a query, we call this routine after
** calling eat_query().  This routine will return the default
** answer which at that point will be in line[].
*/
static void return_default(int sesfd)
	{
	char *ptr;

	DODEBUG_QUERY(("Returning default answer"));

	if((ptr = strchr(line,':')) == (char*)NULL)
		{
		fatal(0, "papsrv_query.c: return_default(): invalid %%%%?Endxxxxxx: yyy\n(%s)",line);
		}

	ptr++;
	while(*ptr==' ')					/* eat up spaces */
		ptr++;

	DODEBUG_QUERY(("default answer: %s",ptr));	  /* we now have answer */

	strcat(line,"\n");					/* put the return back on */

	REPLY(sesfd,ptr);
	} /* end of return_default() */

/*
** Font list query.
**
** Since a query of this type is probably coming from LaserWriter 7.1.2
** or earlier, we must only give the names of printer resident fonts.
** This is because pre-8.0 LaserWriter drivers do not insert comments
** to tell the spooler where to download the fonts.
*/
static void font_list_query(int sesfd, int destid)
	{
	int x;
	char tempstr[75];			/* space for creating font name lines */
	struct ADV *d;

	DODEBUG_QUERY(("font list query"));

	d = &adv[destid];					/* set a pointer to struct for this destination */

	for(x=0;x<d->fontcount;x++)			/* send the whole list */
		{								/* item by item */
		snprintf(tempstr, sizeof(tempstr), "%.64s\n",get_font_name(d->fontlist[x]));
		REPLY(sesfd,tempstr);
		DODEBUG_QUERY(("%s",tempstr));
		}

	REPLY(sesfd,"*\n");					/* send an astrisk to indicate end of reply */

	eat_query(sesfd);					/* eat up the PostScript code and %?End line */
	} /* end of font_list_query() */

/*
** Resource List Query
*/
static void resource_list_query(int sesfd)
	{
	DODEBUG_QUERY(("Resource List Query"));

	tokenize();							/* Parse fields in query line */

	eat_query(sesfd);					/* Throw away the PostScript */

	/* code is missing here */

	REPLY(sesfd, "*\n");				/* Terminate returned list */
	} /* end of resource_list_query() */

/*
** Support routine for use by font_query() and resource_query() when
** the resources in question are fonts.
**
** The client asks about the fonts it is interested in.
** If the font is in the cache we say it is present, otherwise,
** we say it is absent and hope the client will send us a copy.
**
** In the case of dual type (1 and 42) Macintosh TrueType fonts, if we
** don't have both versions, we say we don't have it if the answer
** to the TTRasterizer query is likely to provoke the client to provide
** the missing half.
*/
static void do_font_query(int sesfd, int destid, int index)
	{
	char *type, *space;
	char temp[256];				/* stuff from line[] can't overflow this buffer */
	int x,y;					/* outer and inner loop counters */
	SHORT_INT fontid;			/* ID number of a font, used to see if printer has it */
	int wanted_mactruetype_features;
	int features;

	if(index == 1)				/* FontQuery */
		{
		type = "";				/* needn't say it is a font */
		space = "";				/* no space after colon */
		}
	else						/* ResourceQuery: font */
		{
		type = "font ";			/* must specify type as font */
		space = " ";			/* one space after the colon */
		}

	if(adv[destid].TTRasterizer
				&& (strcmp(adv[destid].TTRasterizer, "Type42") == 0
						|| strcmp(adv[destid].TTRasterizer, "Accept68K")==0))
		wanted_mactruetype_features = FONT_TYPE_42;
	else
		wanted_mactruetype_features = FONT_TYPE_1;

	/* Loop thru the font list */
	for(x=index; tokens[x]; x++)
		{
		/* Check if the font is in the printer. */
		if((fontid = get_font_id(tokens[x])) != -1)		/* If at least one printer has this font, */
			{											/* we will have assigned it an id number. */
			for(y=0; y < adv[destid].fontcount; y++)	/* Compare the ID of each font on this printer */
				{										/* to the font ID we got above. */
				if(adv[destid].fontlist[y]==fontid)		/* If match, */
					{									/* acknowledge that we have it. */
					DODEBUG_QUERY(("%s/%s:%sYes (printer has it)", type, tokens[x], space));
					snprintf(temp, sizeof(temp), "%s/%s:%sYes\n", type, tokens[x], space);
					REPLY(sesfd, temp);
					break;						/* terminate for(y... */
					}
				}
			if(y < adv[destid].fontcount)		/* if printer had it, */
				continue;						/* go on to next font */
			}

		/*
		** If we are allowed to query the cache and if the font is in the cache,
		** and, if this is a two mode Mac TrueType font, we already have the part
		** the client would probably supply if we said "No".
		*/
		if(adv[destid].query_font_cache
				&& noalloc_find_cached_resource("font", tokens[x], 0.0, 0, font_search_list, (int*)NULL, &features, NULL)
				&& ( !(features & FONT_MACTRUETYPE) || (features & wanted_mactruetype_features) ) )
			{
			DODEBUG_QUERY(("%s/%s:%sYes (in cache)", type, tokens[x], space));
			snprintf(temp, sizeof(temp), "%s/%s:%sYes\n", type, tokens[x], space);
			}

		/*
		** Font is not in the cache or we don't have the part the client
		** is likely willing to supply.
		*/
		else
			{
			DODEBUG_QUERY(("%s/%s:No", type, tokens[x]));
			snprintf(temp, sizeof(temp), "%s/%s:No\n", type, tokens[x]);
			}

		REPLY(sesfd, temp);
		}

	REPLY(sesfd, "*\n");		/* Terminate the returned list */
	} /* end of do_font_query() */

/*
** Specific font query.
**
*/
static void font_query(int sesfd, int destid)
	{
	DODEBUG_QUERY(("font query"));

	tokenize();					/* Parse query line to locate words */

	eat_query(sesfd);			/* Throw away the PostScript */

	do_font_query(sesfd, destid, 1);
	} /* end of font_query() */

/*
** Resource query.
**
** This is similiar to font_query(), the diffence being that the
** resource type is the 1st parameter.  I am not sure if this code
** is correct.  Should the name of a procedure set be returned
** with a leading slash, for instance?
*/
static void resource_query(int sesfd, int destid)
	{
	char temp[256];				/* stuff from line[] can't overflow this buffer */
	int x;
	int areprocsets = FALSE;	/* TRUE if resources are procedure sets */
	double version;
	int revision;

	DODEBUG_QUERY(("Resource Query"));

	tokenize();					/* Break the query line into words */

	eat_query(sesfd);			/* Throw away the PostScript */

	/*
	** If the resources in question are fonts, let the special font
	** query answering code handle it.
	*/
	if(strcmp(tokens[1], "font") == 0)
		{
		do_font_query(sesfd, destid, 2);
		return;
		}

	/* If procedure sets, set a flag. */
	if(strcmp(tokens[1], "procset") == 0)
		areprocsets = TRUE;

	/* All right, work through the list. */
	for(x=2; tokens[x]; x++)
		{
		version = 0.0;
		revision = 0;

		/* get procset version and revision */
		if(areprocsets && tokens[x+1] && tokens[x+2])
			{
			version = gu_getdouble(tokens[x+1]);
			revision = atoi(tokens[x+2]);
			x += 2;
			}

		/* Now look in the cache and build a suitable reply in temp[]. */
		if(noalloc_find_cached_resource(tokens[1], tokens[2], version, revision, other_search_list, (int*)NULL, (int*)NULL, NULL))
			{
			snprintf(temp, sizeof(temp), "%s %s: Yes\n", tokens[1], tokens[x]);
			DODEBUG_QUERY(("%s %s: Yes", tokens[1], tokens[x]));
			}
		else
			{
			snprintf(temp, sizeof(temp), "%s %s: No\n", tokens[1], tokens[x]);
			DODEBUG_QUERY(("%s %s: No", tokens[1], tokens[x]));
			}

		REPLY(sesfd, temp);						/* Dispatch the reply */
		}

	REPLY(sesfd, "*\n");						/* Terminate the returned list */
	} /* end of resource_query() */

/*
** Do we have a certain proceedure set?
**
** The problem with this is pre-8.0 LaserWriter drivers
** query for "PatchPrep" but say they are looking for "AppleDict md",
** therefor we generally comment this code out.
*/
#ifdef UNTESTED
void procset_query(int sesfd)
	{
	char *name;
	double version;
	int revision;

	DODEBUG_QUERY(("procset_query(sesfd=%d)", sesfd));

	tokenize();									/* make tokens of line[] */

	eat_query(sesfd);							/* eat up PostScript, etc. */

	name = tokens[1];							/* get procset name */

	if(tokens[2])								/* get version number */
		version=gu_getdouble(tokens[2]);				/* which is a floating point */
	else										/* number; if none present, */
		version=0;								/* user zero */

	if(tokens[3])								/* get revision number */
		sscanf(tokens[3],"%d",&revision);		/* which is an integer */
	else										/* if none present, */
		revision=0;								/* use zero */

	if(noalloc_find_cached_resource("procset", name, version, revision, other_search_list, (int*)NULL, (int*)NULL))
		{
		DODEBUG_QUERY(("procset not present"));
		REPLY(sesfd,"0\n");
		}
	else										/* no error, */
		{										/* the file must be there */
		DODEBUG_QUERY(("procset present"));
		REPLY(sesfd, "1\n");
		}
	} /* end of procset_query() */
#endif

/*
** Feature Query
*/
static void feature_query(int sesfd, int destid)
	{
	char temp[256];

	DODEBUG_QUERY(("feature query: %s", line));

	tokenize();					/* break query into words */

	eat_query(sesfd);			/* throw away the PostScript */

	switch(tokens[1][1])		/* <-- second character */
		{
		case 'L':
			if(strcmp(tokens[1], "*LanguageLevel") == 0)
				{
				snprintf(temp, sizeof(temp), "\"%d\"\n", adv[destid].LanguageLevel != 0 ? adv[destid].LanguageLevel : 1 );
				REPLY(sesfd, temp);
				return;
				}
			break;

		case 'P':
			if(strcmp(tokens[1], "*PSVersion") == 0)
				{
				if(adv[destid].PSVersion)
					{
					snprintf(temp, sizeof(temp), "\"%s\"\n", adv[destid].PSVersion);
					REPLY(sesfd, temp);
					return;
					}
				}
			else if(strcmp(tokens[1], "*Product") == 0)
				{
				if(adv[destid].Product)
					{
					snprintf(temp, sizeof(temp), "\"%s\"\n", adv[destid].Product);
					REPLY(sesfd, temp);
					return;
					}
				}
			break;

		case '?':
			if(strcmp(tokens[1], "*?Resolution") == 0)
				{
				if(adv[destid].Resolution)
					{
					snprintf(temp, sizeof(temp), "%s\n", adv[destid].Resolution);
					REPLY(sesfd, temp);
					return;
					}
				}
			break;

		case 'F':
			if(strcmp(tokens[1], "*FreeVM") == 0)
				{
				if(adv[destid].VMOptionFreeVM != 0)
					{
					snprintf(temp, sizeof(temp), "\"%d\"\n", adv[destid].VMOptionFreeVM);
					REPLY(sesfd, temp);
					return;
					}
				if(adv[destid].FreeVM != 0)		/* If line was present, */
					{
					snprintf(temp, sizeof(temp), "\"%d\"\n", adv[destid].FreeVM);
					REPLY(sesfd, temp);
					return;
					}
				}
			else if(strcmp(tokens[1], "*FaxSupport") == 0)
				{
				if(adv[destid].FaxSupport)
					{
					snprintf(temp, sizeof(temp), "%s\n", adv[destid].FaxSupport);
					REPLY(sesfd, temp);
					return;
					}
				}
			break;

		case 'T':
			if(strcmp(tokens[1], "*TTRasterizer") == 0)
				{
				if(adv[destid].TTRasterizer)
					 {
					 snprintf(temp, sizeof(temp), "%s\n", adv[destid].TTRasterizer);
					 REPLY(sesfd, temp);
					 return;
					 }
				}
			break;				/* return default */

		case 'C':
			if(strcmp(tokens[1], "*ColorDevice") == 0)
				{
				if(adv[destid].ColorDevice == ANSWER_TRUE)
					{
					REPLY(sesfd, "True\n");
					return;
					}
				else if(adv[destid].ColorDevice == ANSWER_FALSE)
					{
					REPLY(sesfd, "False\n");
					return;
					}
				}
			break;

		case 'O':
			if(strncmp(tokens[1], "*Option", 7) == 0)
				{
				struct OPTION *opt;

				opt = adv[destid].options;

				while(opt != (struct OPTION *)NULL)
					{
					if(strcmp(tokens[1], opt->name) == 0)
						{
						snprintf(temp, sizeof(temp), "%s\n", opt->value);
						REPLY(sesfd, temp);
						return;
						}
					opt = opt->next;
					}
				}
			break;

		case 'I':
			if(strcmp(tokens[1], "*InstalledMemory") == 0)
				{
				if(adv[destid].InstalledMemory)
					{
					snprintf(temp, sizeof(temp), "%s\n", adv[destid].InstalledMemory);
					REPLY(sesfd, temp);
					return;
					}
				}
			break;

		} /* end of switch */

	/*
	** Unknown feature query, return the
	** default answer.
	*/
	return_default(sesfd);
	} /* feature_query() */

/*
** Generic query
** We answer a few queries generated by LaserWriter 8.x.
*/
static void generic_query(int sesfd, int destid)
	{
	char temp[256];

	DODEBUG_QUERY(("generic_query(sesfd=%d, destid=%d) line=\"%s\"", sesfd, destid, line));

	tokenize();
	eat_query(sesfd);

	/* Adobe is binary data OK query: */
	if(strcmp(tokens[1], "ADOIsBinaryOK?") == 0)
		{
		if(adv[destid].BinaryOK == ANSWER_TRUE)
			{
			REPLY(sesfd, "True\n");
			return;
			}
		if(adv[destid].BinaryOK == ANSWER_FALSE)
			{
			REPLY(sesfd, "False\n");
			return;
			}
		}

	/* Adobe how much RAM is install query: */
	else if(strcmp(tokens[1], "ADORamSize") == 0)
		{
		if(adv[destid].RamSize)			/* If not zero which indicates unknown, */
			{							/* return the value. */
			snprintf(temp, sizeof(temp), "\"%d\"\n", adv[destid].RamSize);
			REPLY(sesfd,temp);
			return;
			}
		}

	/* University of Michigan list queue supported query. */
	else if(strcmp(tokens[1], "UMICHListQueue") == 0)
		{
		REPLY(sesfd, "true\n");
		return;
		}

	/* University of Michigan cost per page. */
	else if(strcmp(tokens[1], "UMICHCostPerPage") == 0)
		{
		REPLY(sesfd, "10\n");
		return;
		}

	/* University of Michigan delete job supported query. */
	else if(strcmp(tokens[1], "UMICHDeleteJob") == 0)
		{
		REPLY(sesfd,"true\n");
		return;
		}

	/* unrecognized generic query */
	return_default(sesfd);
	} /* end of generic_query() */

/*
** Printer query.  Not yet implemented.
*/
#ifdef UNTESTED
void printer_query(int sesfd, int destid)
	{
	DODEBUG_QUERY(("printer query"));

	eat_query(sesfd);
	return_default(sesfd);
	} /* end of printer_query() */
#endif

/*
** VMstatus query.  Not yet implemented.
*/
#ifdef UNTESTED
void vmstatus_query(int sesfd, int destid)
	{
	DODEBUG_QUERY(("vmstatus_query()"));

	eat_query(sesfd);
	return_default(sesfd);
	} /* end of vmstatus_query() */
#endif

/*
** What user authentication methods do we support?
**
** Well, that depends on which queue.
*/
static void uamethods_query(int sesfd, int destid)
	{
	DODEBUG_QUERY(("uamethods_query(sesfd=%d, destid=%d)", sesfd, destid));

	eat_query(sesfd);

	if( adv[destid].isprotected )
		{
		#ifdef KERBEROS
		REPLY(sesfd,"CleartxtPasswrd\nUMICHKerberosIV\n*\n");
		#else
		REPLY(sesfd,"CleartxtPasswrd\n*\n");
		#endif
		}
	else
		{
		REPLY(sesfd,"NoUserAuthent\n*\n");
		}
	} /* end of uamethods_query() */

/*
** Do a University of Michigan list queue command.
*/
static void do_UMICHListQueue(int sesfd, int destid)
	{
	if(query_trace)
		debug("COMMAND --> %.*s", strcspn(line,"\n"), line);

	REPLY(sesfd,"status: printing\n");
	REPLY(sesfd,".\n");

	REPLY(sesfd,"rank: active\n");
	REPLY(sesfd,"owner: chappell\n");
	REPLY(sesfd,"job: 1404\n");
	REPLY(sesfd,"files: myfile.txt yourfile.txt\n");
	REPLY(sesfd,"size: 123\n");
	REPLY(sesfd,".\n");

	REPLY(sesfd,"rank: 2nd\n");
	REPLY(sesfd,"owner: chappell\n");
	REPLY(sesfd,"job: 1404\n");
	REPLY(sesfd,"files: myfile.txt yourfile.txt\n");
	REPLY(sesfd,"size: 4096\n");
	REPLY(sesfd,".\n");

	REPLY(sesfd,"*\n");


	} /* end of do_UMICHListQueue() */

/*
** Act on a University of Michigan delete job command.
*/
static void do_UMICHDeleteJob(int sesfd, int destid)
	{
	if(query_trace)
		debug("COMMAND --> %.*s", strcspn(line,"\n"), line);

	REPLY(sesfd, "Failed\n");
	/* REPLY(sesfd, "Ok\n"); */

	} /* end of do_UMICHDeleteJob() */

/*
** Read a query job from the client and answer it to the
** best of our ability.  This is called just after the line
** "%!PS-Adobe-x.x Query" is received.
**
** In addition to queries, we handle the "%%Login:" command.
*/
void answer_query(int sesfd, int destid, const char **username, int *preauthorized)
	{
	while(pap_getline(sesfd))			/* `til end of job */
		{
		/* If we should print all PostScript in the query, print this line. */
		if(query_trace >= 2)
			debug("QUERY --> %s", line);

		/* Apple login command */
		if(strncmp(line, "%%Login:", 8) == 0)
			{
			if(query_trace)
				debug("COMMAND --> %.*s", strcspn(line,"\n"), line);

			login_request(sesfd, destid, username, preauthorized);
			continue;
			}

		/* University of Michigan queue list command */
		else if(strcmp(line, "%UMICHListQueue") == 0)
			{
			do_UMICHListQueue(sesfd, destid);
			continue;
			}

		/* University of Michigan delete job. */
		else if(strcmp(line, "%UMICHDeleteJob") == 0)
			{
			do_UMICHDeleteJob(sesfd, destid);
			continue;
			}

		/*
		** If it was not claimed as a command above
		** and it is not a query line, skip it.
		*/
		if(strncmp(line, "%%?", 3))
			continue;

		/* If query trace mode is on, put the line in the log. */
		if(query_trace == 1)
			debug("QUERY --> %s", line);

		/* Old style font list query: */
		if(strcmp(line, "%%?BeginFontListQuery") == 0)
			font_list_query(sesfd,destid);

		/* New style font query: */
		else if(strncmp(line, "%%?BeginFontQuery:", 18) == 0)
			font_query(sesfd,destid);

		/* Old style procset: */
		#ifdef UNTESTED
		else if(strncmp(line, "%%?BeginProcSetQuery:", 21) == 0)
			procset_query(sesfd);
		#endif

		/* Feature query: */
		else if(strncmp(line, "%%?BeginFeatureQuery:", 21) == 0)
			feature_query(sesfd, destid);

		/* Generic query: */
		else if(strncmp(line, "%%?BeginQuery:", 14) == 0)
			generic_query(sesfd, destid);

		/* Printer query: */
		#ifdef UNTESTED
		else if(strcmp(line, "%%?BeginPrinterQuery") == 0)
			printer_query(sesfd, destid);
		#endif

		/* VMStatus query: */
		#ifdef UNTESTED
		else if(strcmp(line, "%%?BeginVMStatus") == 0)
			vmstatus_query(sesfd, destid);
		#endif

		/* Resource Query: */
		else if(strncmp(line, "%%?BeginResourceQuery:", 22) == 0)
			resource_query(sesfd, destid);

		/* User authentication methodes querie: */
		else if(strcmp(line, "%%?BeginUAMethodsQuery") == 0)
			uamethods_query(sesfd, destid);

		/* Unrecognized query, just return the default. */
		else
			{
			eat_query(sesfd);
			return_default(sesfd);
			}

		/* flush_reply(sesfd); */		/* flush reply buffer if possible */
		} /* end of while(not end of file) */

	reply_eoj(sesfd);					/* respond with end of job */
	} /* end of answer_query() */

/* end of file */
