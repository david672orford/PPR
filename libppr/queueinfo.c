/*
** mouse:~ppr/src/libppr/queueinfo.c
** Copyright 1995--2005, Trinity College Computing Center.
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
** Last modified 2 April 2005.
*/

/*+ \file

This module contains the implementation of an object which describes a PPR queue.  An
instance may be created and automatically filled with the attributes of a specific
PPR queue.

*/

#include "config.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "interface.h"
#include "queueinfo.h"

#ifdef TEST
#define DODEBUG(a) { printf a; printf("\n"); }
#else
#define DODEBUG(a) /* noop */
#endif

/* printer information */
struct PRINTER_INFO {
	char *name;
	char *interface;
	struct PPD_PROTOCOLS protocols;
	int feedback;					/* really a boolean */
	int jobbreak;
	int codes;
	gu_boolean binaryOK;
	char *ppdFile;
	char *product;
	char *modelname;
	char *nickname;
	char *shortNickName;
	int psLanguageLevel;
	char *psVersionStr;
	float psVersion;
	int psRevision;
	int psFreeVM;
	char *resolution;
	gu_boolean colorDevice;
	char *faxSupport;
	char *ttRasterizer;
	void *fonts;				/* hash with empty values */
	void *options;				/* hash */
	void *VMOptions;			/* hash */
	};

/* queue information */
struct QUEUE_INFO {
	void *pool;					/* pool to hold allocated memory */
	int debug_level;
	FILE *warnings;
	enum QUEUEINFO_TYPE type;
	char *name;
	char *comment;
	gu_boolean transparentMode;
	gu_boolean psPassThru;
	void *printers;
	gu_boolean common_fonts_found;
	void *fontlist;		/* fonts held in common */
	gu_boolean chargeExists;
	};

/*
 * This function is called for Switchset lines in configuration files.  It
 * looks for anything we want noted and notes it in the QUEUE_INFO structure.
 */
static void do_switchset(struct QUEUE_INFO *qip, char *switchset)
	{
	char *p;
	qip->transparentMode = FALSE;			/* undo previous lines */
	while((p = gu_strsep(&switchset, "|")))
		{
		if(strcmp(p, "Htransparent") == 0 || strcmp(p, "-hack=transparent") == 0)
			qip->transparentMode = TRUE;
		}
	}

/*
 * This notes anything interesting in the PassThru line in the QUEUE_INFO structure.
 */
static void do_passthru(struct QUEUE_INFO *qip, char *list)
	{
	char *p;
	qip->psPassThru = FALSE;				/* undo previous lines */
	while((p = gu_strsep(&list, "|")))
		{
		if(strcmp(p, "postscript") == 0)
			qip->psPassThru = TRUE;
		}
	}

/*
 * This creates a new PRINTER_INFO object and adds it to the list of such
 * objects in a QUEUE_INFO structure.
 */
static struct PRINTER_INFO *do_printer_new_obj(struct QUEUE_INFO *qip, const char name[])
	{
	struct PRINTER_INFO *pip;
	pip = gu_alloc(1, sizeof(struct PRINTER_INFO));
	gu_pca_push(qip->printers, (char*)pip);
	pip->name = gu_strdup(name);
	pip->interface = NULL;
	pip->protocols.TBCP = FALSE;
	pip->protocols.PJL = FALSE;
	pip->feedback = -1;
	pip->jobbreak = JOBBREAK_DEFAULT;
	pip->codes = CODES_DEFAULT;
	pip->binaryOK = FALSE;
	pip->ppdFile = NULL;
	pip->product = NULL;
	pip->modelname = NULL;
	pip->nickname = NULL;
	pip->shortNickName = NULL;
	pip->psLanguageLevel = 1;
	pip->psVersionStr = NULL;
	pip->psVersion = 0.00;
	pip->psRevision = 0;
	pip->psFreeVM = 0;
	pip->resolution = NULL;
	pip->colorDevice = FALSE;
	pip->faxSupport = NULL;
	pip->ttRasterizer = NULL;
	pip->fonts = gu_pch_new(25);
	pip->options = gu_pch_new(12);
	pip->VMOptions = gu_pch_new(6);
	return pip;
	} /* end of do_printer_new_obj() */

/*
 * This function opens a printer's specified PPD file and notes anything we are
 * interested in.
 */
static void do_printer_ppd(struct QUEUE_INFO *qip, struct PRINTER_INFO *pip)
	{
	if(pip->ppdFile)
		{
		void *ppd = ppdobj_new(pip->ppdFile);

		/* These flags are used to ensure that we heed only the first instance. */
		gu_boolean saw_ColorDevice = FALSE;
		gu_boolean saw_LanguageLevel = FALSE;

		if(qip->debug_level > 1)
			printf(_("Extracting information about printer \"%s\" from PPD file \"%s\".\n"), pip->name, pip->ppdFile);

		gu_Try {
			char *line;
			char *p;

			while((line = ppdobj_readline(ppd)))
				{
				if(qip->debug_level > 5)
					printf("PPD: %s\n", line);
				if(line[0] == '*')
					{
					switch(line[1])
						{
						case 'C':
							if((p = lmatchp(line, "*ColorDevice:")))
								{
								if(!saw_ColorDevice)
									{
									if(strcmp(p, "True") == 0)
										pip->colorDevice = TRUE;
									else if(strcmp(p, "False") == 0)
										pip->colorDevice = FALSE;
									else if(qip->warnings)
										fprintf(qip->warnings, _("Warning: PPD file \"%s\" has an invalid ColorDevice value of \"%s\".\n"), pip->ppdFile, p);

									saw_ColorDevice = TRUE;
									}
								}
						case 'D':
							if((p = lmatchp(line, "*DefaultResolution:")) || (p = lmatchp(line, "*DefaultJCLResolution:")))
								{
								/* if not seen yet and looks reasonable */
								if(!pip->resolution)
									{
									if(*p < '0' || *p > '9')
										{
										if(qip->warnings)
											{
											if(lmatch(line, "*DefaultResolution:"))
												fprintf(qip->warnings, _("Warning: PPD file \"%s\" has an invalid %s value of \"%s\".\n"), pip->ppdFile, "DefaultResolution", p);
											else
												fprintf(qip->warnings, _("Warning: PPD file \"%s\" has an invalid %s value of \"%s\".\n"), pip->ppdFile, "DefaultJCLResolution", p);
											}
										}
									else
										{
										/* Replace resolution variants like "600x600dpi" with things like "600dpi". */
										char *p2;
										if((p2 = strchr(p, 'x')))
											{
											int nlen = (p2 - p);
											if(strncmp(p, p + nlen + 1, nlen) == 0 && strcmp(p + nlen + nlen + 1, "dpi") == 0)
												{
												p = p + nlen + 1;
												}
											}
										pip->resolution = gu_strdup(p);
										}
									}
								continue;
								}
						case 'F':
							if((p = lmatchp(line, "*FaxSupport:")))
								{
								if(!pip->faxSupport)
									{
									pip->faxSupport = gu_strdup(p);
									}
								continue;
								}
							if((p = lmatchp(line, "*Font")))
								{
								p = gu_strndup(p, strcspn(p, ":"));
								gu_pch_set(pip->fonts, p, "");	
								continue;
								}
							if((p = lmatchp(line, "*FreeVM:")))
								{
								if(*p == '"' && pip->psFreeVM == 0)
									{
									pip->psFreeVM = atoi(p+1);
									}
								continue;
								}
							break;
						case 'L':
							if((p = lmatchp(line, "*LanguageLevel:")))
								{
								if(*p == '"' && !saw_LanguageLevel)
									{
									pip->psLanguageLevel = atoi(p+1);
									saw_LanguageLevel = TRUE;
									}
								continue;
								}
							break;
						case 'M':
							if((p = lmatchp(line, "*ModelName:")))
								{
								if(*p == '"' && !pip->modelname)
									pip->modelname = ppd_finish_QuotedValue(ppd, p+1);
								continue;
								}
							break;
						case 'N':
							if((p = lmatchp(line, "*NickName:")))
								{
								if(*p == '"' && !pip->nickname)
									{
									pip->nickname = ppd_finish_QuotedValue(ppd, p+1);

									/* special parsing rule */
									if(!pip->shortNickName)
										pip->shortNickName = pip->nickname;
									}
								continue;
								}
							break;
						case 'P':
							if((p = lmatchp(line, "*Product:")))
								{
								if(*p == '"' && !pip->product)
									{
									pip->product = ppd_finish_QuotedValue(ppd, p+1);
									}
								continue;
								}
							if((p = lmatchp(line, "*PSVersion:")))
								{
								if(*p == '"' && !pip->psVersionStr)
									{
									float version;
									int revision;
									p++;
									p[strcspn(p, "\"")] = '\0';
									if(gu_sscanf(p, "(%f) %d", &version, &revision) == 2)
										{
										pip->psVersionStr = gu_strdup(p);
										pip->psVersion = version;
										pip->psRevision = revision;
										}
									}
								continue;
								}
							if((p = lmatchp(line, "*Protocols:")))
								{
								char *f;
								while((f = gu_strsep(&p, " \t")))
									{
									if(strcmp(f, "TBCP") == 0)
										pip->protocols.TBCP = TRUE;
									if(strcmp(f, "PJL") == 0)
										pip->protocols.PJL = TRUE;
									}
								continue;
								}
							break;
						case 'S':
							if((p = lmatchp(line, "*ShortNickName:")))
								{
								if(*p == '"' && !pip->shortNickName)
									{
									pip->shortNickName = ppd_finish_QuotedValue(ppd, p+1);
									}
								continue;
								}
							break;
						case 'T':
							if((p = lmatchp(line, "*TTRasterizer:")))
								{
								if(!pip->ttRasterizer)
									{
									pip->ttRasterizer = gu_strdup(p);
									}
								continue;
								}
							break;
						case 'V':
							if((p = lmatchp(line, "*VMOption ")))
								{
								char *name = gu_strndup(p, strcspn(p, "/:"));
								p += strcspn(p, ":");
								if(*p == ':')
									{
									p++;
									p += strspn(p, " \t");
									if(*p == '"')
										{
										p++;
										gu_pch_set(pip->VMOptions, name, gu_strndup(p, strcspn(p, "\"")));	
										}
									}
								continue;
								}
							break;
						}
					}
				}

			/* If these wern't specified in the configuration file, choose defaults based 
			 * on the interface and supported protocols as indicated in the PPD file.
			 */
			if(pip->feedback == -1)
				pip->codes = interface_default_codes(pip->interface, &pip->protocols);
			if(pip->jobbreak == JOBBREAK_DEFAULT)
				pip->codes = interface_default_codes(pip->interface, &pip->protocols);
			if(pip->codes == CODES_DEFAULT)
				pip->codes = interface_default_codes(pip->interface, &pip->protocols);

			/* These two codes settings mean that any 8 bit value can be passed to the
			 * PostScript interpreter without being interpreted as a control code.
			 */
			if(pip->codes == CODES_Binary || pip->codes == CODES_TBCP)
				pip->binaryOK = TRUE;
		
			/* Is a memory expansion module installed? */
			{
			const char *name;
			if((name = gu_pch_get(pip->options, "*InstalledMemory")))
				{
				const char *value_string;
				if((value_string = gu_pch_get(pip->VMOptions, name)))
					{
					pip->psFreeVM = atoi(value_string);
					}
				}
			}
			}
		gu_Final {
			ppdobj_free(ppd);
			}
		gu_Catch {
			gu_ReThrow();
			}
		}
	} /* end of do_printer_ppd() */

static gu_boolean do_printer(struct QUEUE_INFO *qip, const char name[], int depth)
	{
	char fname[MAX_PPR_PATH];
	FILE *conf;
	char *line = NULL;
	int line_available = 80;
	char *p;
	struct PRINTER_INFO *pip;

	DODEBUG(("do_printer(qip, name[]=\"%s\", depth=%d)", name, depth));
	
	ppr_fnamef(fname, "%s/%s", PRCONF, name);
	if(!(conf = fopen(fname, "r")))
		return FALSE;

	pip = do_printer_new_obj(qip, name);

	while((line = gu_getline(line, &line_available, conf)))
		{
		DODEBUG(("line: %s", line));
		switch(line[0])
			{
			case 'C':
				if((p = lmatchp(line, "Comment:")) && depth == 0)
					{
					qip->comment = gu_strdup(p);
					continue;
					}
				if((p = lmatchp(line, "Codes:")))
					{
					pip->codes = atoi(p);
					continue;
					}
				if((p = lmatchp(line, "Charge:")))
					{
					qip->chargeExists = TRUE;
					}
				break;
			case 'F':
				if((p = lmatchp(line, "Feedback:")))
					{
					gu_torf_setBOOL(&pip->feedback, p);
					continue;
					}
				break;
			case 'I':
				if((p = lmatchp(line, "Interface:")))
					{
					pip->interface = gu_strdup(p);
					/* Invalidate lines which preceed "Interface:". */
					pip->feedback = -1;
					pip->jobbreak = JOBBREAK_DEFAULT;
					pip->codes = CODES_DEFAULT;
					continue;
					}
				break;
			case 'J':
				if((p = lmatchp(line, "JobBreak:")))
					{
					pip->jobbreak = atoi(p);
					continue;
					}
				break;
			case 'P':
				if((p = lmatchp(line, "PPDFile:")))
					{
					pip->ppdFile = gu_strdup(p);
					continue;
					}
				if((p = lmatchp(line, "PPDOpt:")))
					{
					char *name, *value;
					if((name = gu_strsep(&p," ")) && (value = gu_strsep(&p," ")))
						{
						name = gu_strdup(name);
						value = gu_strdup(value);
						gu_pch_set(pip->options, name, value);
						}
					continue;
					}
				if((p = lmatchp(line, "PassThru:")))
					{
					if(depth == 0)
						do_passthru(qip, p);
					continue;
					}
				break;
			case 'S':
				if((p = lmatchp(line, "Switchset:")))
					{
					if(depth == 0)
						do_switchset(qip, p);
					continue;
					}
				break;
			}
		}

	fclose(conf);

	/* Parse the PPD file and load info into this PRINTER_INFO structure. */
	do_printer_ppd(qip, pip);

	return TRUE;
	} /* end of do_printer() */

static gu_boolean do_group(struct QUEUE_INFO *qip, const char name[], int depth)
	{
	char fname[MAX_PPR_PATH];
	FILE *conf;
	char *line = NULL;
	int line_available = 80;
	char *p;

	DODEBUG(("do_group(qip, name[]=\"%s\", depth=%d)", name, depth));

	ppr_fnamef(fname, "%s/%s", GRCONF, name);
	if(!(conf = fopen(fname, "r")))
		return FALSE;

	while((line = gu_getline(line, &line_available, conf)))
		{
		DODEBUG(("line: %s", line));
		switch(line[0])
			{
			case 'C':
				if((p = lmatchp(line, "Comment:")) && depth == 0)
					{
					qip->comment = gu_strdup(p);
					continue;
					}
				break;
			case 'P':
				if((p = lmatchp(line, "Printer:")))
					{
					do_printer(qip, p, depth + 1);
					continue;
					}
				if((p = lmatchp(line, "PassThru:")))
					{
					do_passthru(qip, p);
					continue;
					}
				break;
			case 'S':
				if((p = lmatchp(line, "Switchset:")))
					{
					do_switchset(qip, p);
					continue;
					}
				break;
			}
		}

	fclose(conf);
	return TRUE;
	} /* end of do_group() */

static gu_boolean do_alias(struct QUEUE_INFO *qip)
	{
	char fname[MAX_PPR_PATH];
	FILE *conf;
	char *line = NULL;
	int line_available = 80;
	char *p;

	DODEBUG(("do_alias(qip): qip->name=\"%s\")", qip->name));

	ppr_fnamef(fname, "%s/%s", ALIASCONF, qip->name);
	if(!(conf = fopen(fname, "r")))
		return FALSE;

	while((line = gu_getline(line, &line_available, conf)))
		{
		DODEBUG(("line: %s", line));
		switch(line[0])
			{
			case 'C':
				if((p = lmatchp(line, "Comment:")))
					{
					qip->comment = gu_strdup(p);
					continue;
					}
				break;
			case 'F':
				if((p = lmatchp(line, "ForWhat:")))
					{
					if(!(do_group(qip, p, 1) || do_printer(qip, p, 1)))
						gu_Throw("broken alias");
					continue;
					}
				break;
			case 'P':
				if((p = lmatchp(line, "PassThru:")))
					{
					do_passthru(qip, p);
					continue;
					}
				break;
			case 'S':
				if((p = lmatchp(line, "Switchset:")))
					{
					do_switchset(qip, p);
					continue;
					}
				break;
			}
		}

	fclose(conf);
	return TRUE;
	} /* end of do_alias() */

/** create a queueinfo object
*/
void *queueinfo_new(enum QUEUEINFO_TYPE qit, const char name[])
	{
	void *pool = gu_pool_new();
	struct QUEUE_INFO *qip = gu_alloc(1, sizeof(struct QUEUE_INFO));
	qip->pool = pool;
	GU_OBJECT_POOL_PUSH(qip->pool);
	qip->debug_level = 0;
	qip->warnings = NULL;
	qip->type = qit;
	qip->name = gu_strdup(name);
	qip->comment = NULL;
	qip->transparentMode = FALSE;
	qip->psPassThru = FALSE;
	qip->printers = gu_pca_new(8, 8);
	qip->common_fonts_found = FALSE;
	qip->fontlist = gu_pca_new(50, 50);
	qip->chargeExists = FALSE;
	GU_OBJECT_POOL_POP(qip->pool);
	return (void*)qip;
	} /* end of queueinfo_new() */

/** discard a queueinfo object
*/
void queueinfo_free(void *p)
	{
	struct QUEUE_INFO *qip = (struct QUEUE_INFO *)p;
	gu_pool_free(qip->pool);
	}

/** create a queueinfo object and load a queue's information into it
*/
void *queueinfo_new_load_config(enum QUEUEINFO_TYPE qit, const char name[])
	{
	struct QUEUE_INFO *qip = queueinfo_new(qit, name);
	gu_pool_push(qip->pool);
	gu_Try {
		switch(qip->type)
			{
			case QUEUEINFO_SEARCH:
				if(do_alias(qip))
					qip->type = QUEUEINFO_ALIAS;
				else if(do_group(qip, qip->name, 0))
					qip->type = QUEUEINFO_GROUP;
				else if(do_printer(qip, qip->name, 0))
					qip->type = QUEUEINFO_PRINTER;
				else
					gu_Throw("There is no alias, group, or printer called \"%s\"", qip->name);
				break;
	
			case QUEUEINFO_ALIAS:
				if(!do_alias(qip))
					gu_CodeThrow(EEXIST, _("There is no alias called \"%s\""), qip->name);
				break;
	
			case QUEUEINFO_GROUP:
				if(!do_group(qip, qip->name, 0))
					gu_CodeThrow(EEXIST, _("There is no group called \"%s\""), qip->name);
				break;
	
			case QUEUEINFO_PRINTER:
				if(!do_printer(qip, qip->name, 0))
					gu_CodeThrow(EEXIST, _("There is no printer called \"%s\""), qip->name);
				break;
			}
		}
	gu_Final {
		gu_pool_pop(qip->pool);
		}
	gu_Catch {
		queueinfo_free(qip);
		gu_ReThrow();
		}
	return (void *)qip;
	} /* end of queueinfo_new_load_config() */

/** add a printer to the object
 *
 * This is used to load information about printers which are about to be 
 * added to a group.
*/
void queueinfo_add_printer(void *p, const char name[])
	{
	struct QUEUE_INFO *qip = (struct QUEUE_INFO *)p;
	GU_OBJECT_POOL_PUSH(qip->pool);
	if(!do_printer(qip, name, 0))
		gu_CodeThrow(EEXIST, _("no printer called \"%s\""), name);
	GU_OBJECT_POOL_POP(qip->pool);
	}

/** add a hypothetical printer to this object
 *
 * This function loads the indicated PPD file as if it belonged to one of the
 * member printers of this object.  This capability is used to build queueinfo
 * objects for printers and group which haven't been created yet.
*/
void queueinfo_add_hypothetical_printer(void *p, const char name[], const char ppdfile[], const char installed_memory[])
	{
	struct QUEUE_INFO *qip = (struct QUEUE_INFO *)p;
	struct PRINTER_INFO *pip;
	GU_OBJECT_POOL_PUSH(qip->pool);
	pip = do_printer_new_obj(qip, name);
	pip->ppdFile = gu_strdup(ppdfile);
	if(installed_memory)
		gu_pch_set(pip->options, "*InstalledMemory", gu_strdup(installed_memory));
	do_printer_ppd(qip, pip);
	GU_OBJECT_POOL_POP(qip->pool);
	}

/** Set a file object to which warning messages may be sent
 *
 * The queueinfo object can spew warnings about problems with the
 * PPD file and the like.  These are off by default, but you can
 * turn them on by supplying a FILE object (such as stderr) to 
 * which to send them.  This feature is used by ppad(1).
 */
void queueinfo_set_warnings_file(void *p, FILE *warnings)
	{
	struct QUEUE_INFO *qip = (struct QUEUE_INFO *)p;
	qip->warnings = warnings;
	}

/** Set a debug level for debug messages to stdout
 *
 * If the debug level is set to non-zero, then messages describing
 * the parsing process will be written to stdout.  Obviously you
 * should only do this if there is a stdout to write to.  In other
 * words, you probably don't want to do this in a daemon.
 */
void queueinfo_set_debug_level(void *p, int debug_level)
	{
	struct QUEUE_INFO *qip = (struct QUEUE_INFO *)p;
	qip->debug_level = debug_level;
	}

/** return the name of the queue
*/
const char *queueinfo_name(void *p)
	{
	struct QUEUE_INFO *qip = (struct QUEUE_INFO *)p;
	return qip->name;
	}

/** return the description of the queue
 *
 * The description is the one set with "ppad (group, alias) comment".
*/
const char *queueinfo_comment(void *p)
	{
	struct QUEUE_INFO *qip = (struct QUEUE_INFO *)p;
	return qip->comment;
	}

/** Is the queue in transparent mode?
 *
 * This function returns TRUE if "-H transparent" is in the switchset.
*/
gu_boolean queueinfo_transparentMode(void *p)
	{
	struct QUEUE_INFO *qip = (struct QUEUE_INFO *)p;
	return qip->transparentMode;
	}

/** Will the queue pass PostScript documents through unchanged?
 *
 * This function returns TRUE if "postscript" is in the list set
 * with "ppad (group, alias) passthru".
*/
gu_boolean queueinfo_psPassThru(void *p)
	{
	struct QUEUE_INFO *qip = (struct QUEUE_INFO *)p;
	return qip->psPassThru;
	}

/** Can all possible 8-bit character codes be passed thru to the PS interpreter?
 *
 * We return TRUE if all member printers can pass all 8-bit codes.  If there
 * are no member printers, we return FALSE.  A "ppad codes" setting of "Binary"
 * or "TBCP".
*/
gu_boolean queueinfo_binaryOK(void *p)
	{
	struct QUEUE_INFO *qip = (struct QUEUE_INFO *)p;
	int i;
	gu_boolean answer = FALSE;

	for(i=0; i < gu_pca_size(qip->printers); i++)
		{
		struct PRINTER_INFO *pip;
		pip = gu_pca_index(qip->printers, i);
		if(!pip->binaryOK)
			return FALSE;
		else
			answer = TRUE;
		}

	return answer;
	}

/** Return the name of the queue's printer(s)'s PPD file
 *
 * If these is more than one printer and not all have the same PPD file, then
 * this function returns NULL.
*/
const char *queueinfo_ppdFile(void *p)
	{
	struct QUEUE_INFO *qip = (struct QUEUE_INFO *)p;
	struct PRINTER_INFO *pip;
	int i;
	const char *answer = NULL;

	for(i=0; i < gu_pca_size(qip->printers); i++)
		{
		pip = gu_pca_index(qip->printers, i);
		if(!pip->ppdFile)
			return NULL;
		if(!answer)
			answer = pip->ppdFile;
		else if(strcmp(answer, pip->ppdFile))
			return NULL;
		}

	return answer;
	}

/** read the PostScript product string of the queue's printer(s)
 *
 * If these is more than one printer and not all have the same product string, then
 * this function returns NULL.
*/
const char *queueinfo_product(void *p)
	{
	struct QUEUE_INFO *qip = (struct QUEUE_INFO *)p;
	struct PRINTER_INFO *pip;
	int i;
	const char *answer = NULL;

	for(i=0; i < gu_pca_size(qip->printers); i++)
		{
		pip = gu_pca_index(qip->printers, i);
		if(!pip->product)
			return NULL;
		if(!answer)
			answer = pip->product;
		else if(strcmp(answer, pip->product))
			return NULL;
		}

	return answer;
	}

/** read the PostScript ShortNickName string of the queue's printer(s)
 *
 * If these is more than one printer and not all have the same product string, then
 * this function returns NULL.
*/
const char *queueinfo_shortNickName(void *p)
	{
	struct QUEUE_INFO *qip = (struct QUEUE_INFO *)p;
	struct PRINTER_INFO *pip;
	int i;
	const char *answer = NULL;

	for(i=0; i < gu_pca_size(qip->printers); i++)
		{
		pip = gu_pca_index(qip->printers, i);
		if(!pip->shortNickName)
			return NULL;
		if(!answer)
			answer = pip->shortNickName;
		else if(strcmp(answer, pip->shortNickName))
			return NULL;
		}

	return answer;
	}

/*
 * Find the member printer with the oldest PostScript interpreter.  Several
 * functions below use this information.
 */
static struct PRINTER_INFO *find_lowest_version(struct QUEUE_INFO *qip)
	{
	int i;
	struct PRINTER_INFO *pip, *lowest = NULL;

	for(i=0; i < gu_pca_size(qip->printers); i++)
		{
		pip = gu_pca_index(qip->printers, i);
		if(!pip->psVersion)
			return NULL;
		if(!lowest || pip->psVersion < lowest->psVersion 
				|| (pip->psVersion == lowest->psVersion && pip->psRevision < lowest->psRevision))
			{
			lowest = pip;
			}
		}

	return lowest;
	}

/** read the PostScript language level of the queue's printer(s)
 *
 * If there is more than one printer, the lowest language level supported
 * is returned.
*/
int queueinfo_psLanguageLevel(void *p)
	{
	struct QUEUE_INFO *qip = (struct QUEUE_INFO *)p;
	struct PRINTER_INFO *pip = find_lowest_version(qip);
	if(pip)
		return pip->psLanguageLevel;
	else
		return 0;		/* unknown */
	}

/** What is the version string of the PostScript interpreter?
 *
 * The version string is returned exactly as it appears in the PPD file.
 * If the version string isn't available, this function will return NULL.
*/
const char *queueinfo_psVersionStr(void *p)
	{
	struct QUEUE_INFO *qip = (struct QUEUE_INFO *)p;
	struct PRINTER_INFO *pip = find_lowest_version(qip);
	if(pip)
		return pip->psVersionStr;
	else
		return NULL;	/* unknown */
	}

/** What is the version number of the PostScript interpreter?
 *
 * If it can't determine the answer, this function returns 0.0.  A printer 
 * driver might need to know this to determine which language features are
 * available.  Note that the value is stored in a float, so it may differ
 * slightly from the number in the PPD file.
*/
double queueinfo_psVersion(void *p)
	{
	struct QUEUE_INFO *qip = (struct QUEUE_INFO *)p;
	struct PRINTER_INFO *pip = find_lowest_version(qip);
	if(pip)
		return pip->psVersion;
	else
		return 0.0;
	}

/** What is the revision number of the printer-specific parts of the PostScript interpreter?
 *
 * If it can't determine the answer, this function returns 0 (which is also 
 * a value).  This number is an integer.  While the PostScript version number
 * indicates the version of the langauge implemenation, this number indicates
 * the version of the software which integrates the stated PostScript version of
 * the PostScript intepreter with a particular printer's other firmware.
*/
int queueinfo_psRevision(void *p)
	{
	struct QUEUE_INFO *qip = (struct QUEUE_INFO *)p;
	struct PRINTER_INFO *pip = find_lowest_version(qip);
	if(pip)
		return pip->psRevision;
	else
		return 0;
	}

/** How many bytes of PostScript memory are available to programs?
 *
 * If this function can't determine the answer, it returns 0.
*/
int queueinfo_psFreeVM(void *p)
	{
	struct QUEUE_INFO *qip = (struct QUEUE_INFO *)p;
	struct PRINTER_INFO *pip;
	int i;
	int lowest_freevm = 0;

	for(i=0; i < gu_pca_size(qip->printers); i++)
		{
		pip = gu_pca_index(qip->printers, i);
		if(pip->psFreeVM == 0)
			return 0;
		if(lowest_freevm == 0 || pip->psFreeVM < lowest_freevm)
			lowest_freevm = pip->psFreeVM;
		}

	return lowest_freevm;
	}


/** What is the resolution (expressed as a string)?
 *
 * We will return the lowest resolution of all of the printers.  In
 * determining the lowest resolution, we will examine only the first
 * number.  Thus, "300x1200dpi" will be considered lower than "360dpi".
*/
const char *queueinfo_resolution(void *p)
	{
	struct QUEUE_INFO *qip = (struct QUEUE_INFO *)p;
	struct PRINTER_INFO *pip;
	int i;
	int lowest_resolution = 0;
	char *lowest_resolution_string = NULL;

	for(i=0; i < gu_pca_size(qip->printers); i++)
		{
		pip = gu_pca_index(qip->printers, i);
		if(!pip->resolution)
			return 0;
		if(lowest_resolution == 0 || atoi(pip->resolution) < lowest_resolution)
			{
			lowest_resolution = atoi(pip->resolution);
			lowest_resolution_string = pip->resolution;
			}
		}

	return lowest_resolution_string;
	}

/** Can the printer print in color?
*/
gu_boolean queueinfo_colorDevice(void *p)
	{
	struct QUEUE_INFO *qip = (struct QUEUE_INFO *)p;
	int i;
	gu_boolean answer = FALSE;

	for(i=0; i < gu_pca_size(qip->printers); i++)
		{
		struct PRINTER_INFO *pip;
		pip = gu_pca_index(qip->printers, i);
		if(!pip->colorDevice)
			return FALSE;
		else
			answer = TRUE;
		}

	return answer;
	}

/** What kind of fax support does the printer have, if any?
 *
 * This is a string such as "None" or "Base".  If one or more printers
 * doesn't specify its fax support or they don't all have the same
 * type then this function will return NULL.
*/
const char *queueinfo_faxSupport(void *p)
	{
	struct QUEUE_INFO *qip = (struct QUEUE_INFO *)p;
	struct PRINTER_INFO *pip;
	int i;
	const char *answer = NULL;

	for(i=0; i < gu_pca_size(qip->printers); i++)
		{
		pip = gu_pca_index(qip->printers, i);
		if(!pip->faxSupport)
			return NULL;
		else if(!answer)
			answer = pip->faxSupport;
		else if(strcmp(answer, pip->faxSupport))
			return NULL;
		}

	return answer;
	}

/** What kind of TrueType rasterizer does the printer have, if any?
 *
 * This will be either "None", "Type42", or "68000" (or something like that).
 * If one or more printers doesn't specify this value or they don't 
 * specify the same value, then we return NULL.
*/
const char *queueinfo_ttRasterizer(void *p)
	{
	struct QUEUE_INFO *qip = (struct QUEUE_INFO *)p;
	struct PRINTER_INFO *pip;
	int i;
	const char *answer = NULL;

	for(i=0; i < gu_pca_size(qip->printers); i++)
		{
		pip = gu_pca_index(qip->printers, i);
		if(!pip->ttRasterizer)
			return NULL;
		else if(!answer)
			answer = pip->ttRasterizer;
		else if(strcmp(answer, pip->ttRasterizer))
			return NULL;
		}

	return answer;
	}

/** does it cost money to print on any of the printers?

*/
gu_boolean queueinfo_chargeExists(void *p)
	{
	struct QUEUE_INFO *qip = (struct QUEUE_INFO *)p;
	return qip->chargeExists;
	}
  
/*
** Create a font list which includes all fonts which the printers hold in common.
*/
static void find_common_fonts(struct QUEUE_INFO *qip)
	{
	if(!qip->common_fonts_found)
		{
		GU_OBJECT_POOL_PUSH(qip->pool);
		if(gu_pca_size(qip->printers) > 0)
			{
			struct PRINTER_INFO *pip, *pipy;
			int y;
			char *fontname;

			pip = gu_pca_index(qip->printers, 0);
			for(gu_pch_rewind(pip->fonts); (fontname = gu_pch_nextkey(pip->fonts,NULL)); )
				{
				for(y=1; y < gu_pca_size(qip->printers); y++)
					{
					pipy = gu_pca_index(qip->printers, y);
					if(!gu_pch_get(pipy->fonts, fontname))
						break;
					}
				if(y == gu_pca_size(qip->printers))
					gu_pca_push(qip->fontlist, fontname);
				}
			}
		qip->common_fonts_found = TRUE;
		GU_OBJECT_POOL_POP(qip->pool);
		}
	} /* end of find_common_fonts() */

/** How many fonts are in the font list?
*/
int queueinfo_fontCount(void *p)
	{
	struct QUEUE_INFO *qip = (struct QUEUE_INFO *)p;
	find_common_fonts(qip);
	return gu_pca_size(qip->fontlist);
	}

/** Return item index from the printer's font list
*/
const char *queueinfo_font(void *p, int index)
	{
	struct QUEUE_INFO *qip = (struct QUEUE_INFO *)p;
	find_common_fonts(qip);
	if(index > gu_pca_size(qip->fontlist))
		return NULL;
	return gu_pca_index(qip->fontlist, index);
	}

/** Check if a specified font exists
 *
 * !!! exists where ???
*/
gu_boolean queueinfo_fontExists(void *p, const char name[])
	{
	struct QUEUE_INFO *qip = (struct QUEUE_INFO *)p;
	struct PRINTER_INFO *pip;
	int i;
	find_common_fonts(qip);
	for(i=0; i < gu_pca_size(qip->printers); i++)
		{
		pip = gu_pca_index(qip->printers, i);
		if(!gu_pch_get(pip->fonts, name))
			return FALSE;
		}
	return TRUE;
	}

/** Return the value of a printer optional equipment option
 *
 * We can only return the option value if all printers use the same
 * PPD file and all have the same value for the option.
*/
const char *queueinfo_optionValue(void *p, const char name[])
	{
	struct QUEUE_INFO *qip = (struct QUEUE_INFO *)p;
	struct PRINTER_INFO *pip;
	int i;
	const char *answer = NULL;
	const char *value;
	if(!queueinfo_ppdFile(p))
		return NULL;
	for(i=0; i < gu_pca_size(qip->printers); i++)
		{
		pip = gu_pca_index(qip->printers, i);
		if(!(value = gu_pch_get(pip->options, name)))
			return FALSE;
		if(!answer)
			answer = value;
		else if(strcmp(answer, value))
			return NULL;
		}
	return answer;
	}

/*
** Look up a printer's product, modelname, nickname, and resolution and return a MetaFont mode.
*/
static char *get_mfmode(struct QUEUE_INFO *qip, struct PRINTER_INFO *pip)
	{
	FILE *modefile;
	char *line = NULL;
	int line_space_available = 80;		/* suggested initial line buffer size */
	int linenum;
	const char *p, *m, *n, *r;			/* for cleaned up arguments */
	char *f[5];							/* for line fields */
	char *ptr;
	int x;
	char *answer = (char*)NULL;

	/* Assign short variable names and replace NULL pointers
	   with zero-length strings. */
	p = pip->product ? pip->product : "";
	m = pip->modelname ? pip->modelname : "";
	n = pip->nickname ? pip->nickname : "";
	r = pip->resolution ? pip->resolution : "";

	if(qip->debug_level >= 2)
		{
		printf(X_("Looking up mfmode for product=\"%s\",\n"
					"\tmodelname=\"%s\", nickname=\"%s\",\n"
					"\tresolution \"%s\" in \"%s\".\n"), p, m, n, r, MFMODES);
		}

	if((modefile = fopen(MFMODES, "r")) == (FILE*)NULL)
		gu_Throw(_("Can't open \"%s\", errno=%d (%s)"), MFMODES, errno, gu_strerror(errno));

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
			if(qip->warnings)
				fprintf(qip->warnings, _("Warning: syntax error in \"%s\" line %d: too few fields\n"), MFMODES, linenum);
			continue;
			}

		for(x=0; x < (sizeof(f) / sizeof(f[0])); x++)
			{
			if(f[x][0] == '\0')
				{
				if(qip->warnings)
					fprintf(qip->warnings, _("Warning: syntax error in \"%s\" line %d: field %d is empty\n"), MFMODES, linenum, x);
				}
			}
		if(x < (sizeof(f) / sizeof(f[0]))) continue;	/* skip line if error detected in the for() loop */

		if(qip->debug_level >= 3)
			printf(X_("line %d: product=\"%s\", modelname=\"%s\", nickname=\"%s\", resolution=\"%s\", mfmode=\"%s\"\n"), linenum, f[0], f[1], f[2], f[3], f[4]);

		if(f[0][0] != '*' && strcmp(p, f[0]))
			continue;

		if(f[1][0] != '*' && strcmp(m, f[1]))
			continue;

		if(f[2][0] != '*' && strcmp(n, f[2]))
			continue;

		if(f[3][0] != '*' && strcmp(r, f[3]))
			continue;

		answer = gu_strdup(f[4]);

		if(qip->debug_level >= 2)
			printf(_("Match at \"%s\" line %d, mfmode=\"%s\".\n"), MFMODES, linenum, answer);

		break;
		}

	if(line)
		gu_free(line);

	{
	int error = ferror(modefile);
	fclose(modefile);
	if(error)
		gu_Throw(_("Error reading \"%s\", errno=%d (%s)"), MFMODES, errno, gu_strerror(errno));
	}

	return answer;
	} /* end of get_mfmode() */

/** Figure out the MetaFont mode for this queue
 */
const char *queueinfo_computedMetaFontMode(void *p)
	{
	struct QUEUE_INFO *qip = (struct QUEUE_INFO *)p;
	struct PRINTER_INFO *pip;
	int i;
	char *answer = NULL;
	char *temp;

	GU_OBJECT_POOL_PUSH(qip->pool);

	for(i=0; i < gu_pca_size(qip->printers); i++)
		{
		pip = gu_pca_index(qip->printers, i);
		if(!(temp = get_mfmode(qip, pip)))
			{
			if(qip->warnings)
				fprintf(qip->warnings, _("Warning: no MetaFont mode found for printer \"%s\".\n"), pip->name);
			answer = NULL;
			break;
			}
		else if(!answer)
			{
			answer = temp;
			}
		else if(strcmp(answer, temp))
			{
			if(qip->warnings)
				fprintf(qip->warnings, _("Warning: not all members of group \"%s\" have the save MetaFont mode.\n"), qip->name);
			answer = NULL;
			break;
			}
		}
	if(answer)
		answer = gu_pool_return(answer);

	GU_OBJECT_POOL_POP(qip->pool);

	return answer;
	} /* end of queueinfo_computedMetaFontMode() */
	
/** Compute the default filter options for this queue
 *
 * I have considered changing this code to use a PCS, but there doesn't seem
 * to be a real need.  The length of the default filter options is limited
 * in a practical sense and the temporary array is more than large enough.
 * Since it is allocated on the stack, it is cleaned up perfectly without
 * fragmenting the heap.
 */
const char *queueinfo_computedDefaultFilterOptions(void *p)
	{
	struct QUEUE_INFO *qip = (struct QUEUE_INFO *)p;
	char result_line[256];
	char *retval = NULL;		/* nix spurious warning */
	const char *sp;
	int i;
	
	GU_OBJECT_POOL_PUSH(qip->pool);

	snprintf(result_line, sizeof(result_line),
		"level=%d colour=%s",
		queueinfo_psLanguageLevel(p),
		queueinfo_colorDevice(p) ? "True" : "False"
		);

	if((sp = queueinfo_resolution(p)))
		{
		gu_snprintfcat(result_line, sizeof(result_line), " resolution=%.*s",
			(int)strspn(sp, "0123456789"), sp
			);
		}

	if((i = queueinfo_psFreeVM(p)) > 0)
		gu_snprintfcat(result_line, sizeof(result_line), " freevm=%d", i);

	if((sp = queueinfo_computedMetaFontMode(p)))
		gu_snprintfcat(result_line, sizeof(result_line), " mfmode=%s", sp);

	if(qip->debug_level >= 2)
		printf("New default filter options for \"%s\" are: %s\n", qip->name, result_line);

	retval = gu_strdup(result_line);

	GU_OBJECT_POOL_POP(qip->pool);
	
	return retval;
	} /* end of queueinfo_computedDefaultFilterOptions() */

/*
** Test program
** gcc -Wall -DTEST -I../include -o queueinfo queueinfo.c ../libppr.a ../libgu.a -lz
*/
#ifdef TEST
int main(int argc, char *argv[])
	{
	void *obj;
	const char *p;
	int i;
	if(argc != 2)
		{
		fprintf(stderr, "%s: missing argument\n", argv[0]);
		return 1;
		}
	obj = queueinfo_new_load_config(QUEUEINFO_SEARCH, argv[1]);
	printf("name: %s\n", queueinfo_name(obj));
	printf("comment: %s\n", queueinfo_comment(obj));
	printf("transparent mode: %s\n", queueinfo_transparentMode(obj) ? "true" : "false");
	printf("PS passthru: %s\n", queueinfo_psPassThru(obj) ? "true" : "false");
	printf("binary ok: %s\n", queueinfo_binaryOK(obj) ? "true" : "false");
	printf("PPD file: %s\n", (p = queueinfo_ppdFile(obj)) ? p : "<NULL>");
	printf("product: %s\n", (p = queueinfo_product(obj)) ? p : "<NULL>");
	printf("languagelevel: %d\n", queueinfo_psLanguageLevel(obj));
	printf("psVersionStr[] = \"%s\"\n", queueinfo_psVersionStr(obj));	
	printf("psVersion[] = %f\n", queueinfo_psVersion(obj));	
	printf("psRevision[] = %d\n", queueinfo_psRevision(obj));	
	printf("freeVM = %d\n", queueinfo_psFreeVM(obj));
	printf("resolution = %s\n", queueinfo_resolution(obj));
	printf("colorDevice = %s\n", queueinfo_colorDevice(obj) ? "true" : "false");
	printf("faxSupport = %s\n", queueinfo_faxSupport(obj));
	printf("ttRasterizer = %s\n", queueinfo_ttRasterizer(obj));
	printf("font count = %d\n", queueinfo_fontCount(obj));
	for(i=0; i < queueinfo_fontCount(obj); i++)
		printf("\t\"%s\"\n", queueinfo_font(obj, i));
	printf("font_exists[Times-Roman] = %s\n", queueinfo_fontExists(obj, "Times-Roman") ? "true" : "false");
	printf("font_exists[Donald-Duck] = %s\n", queueinfo_fontExists(obj, "Donald-Duck") ? "true" : "false");
	printf("*Option1 = \"%s\"\n", queueinfo_optionValue(obj, "*Option1"));
	printf("*InstalledMemory = \"%s\"\n", queueinfo_optionValue(obj, "*InstalledMemory"));
	printf("mfmode = %s\n", queueinfo_computedMetaFontMode(obj));
	printf("filter_options = \"%s\"\n", queueinfo_computedDefaultFilterOptions(obj));
	return 0;
	}
#endif

/* end of file */
