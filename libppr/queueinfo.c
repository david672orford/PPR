/*
** mouse:~ppr/src/libppr/queueinfo.c
** Copyright 1995--2004, Trinity College Computing Center.
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
** Last modified 30 January 2004.
*/

/*+ \file

This module contains the implementation of an object which describes a PPR queue.  An
instance may be created and automatically filled with the attributes of a specific
PPR queue.

*/

#include "before_system.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include "gu.h"
#include "global_defines.h"
#include "interface.h"
#include "queueinfo.h"

#include "pool.h"
#include "pstring.h"
#include "vector.h"
#include "hash.h"

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
	int psLanguageLevel;
	char *psVersionStr;
	float psVersion;
	int psRevision;
	int psFreeVM;
	char *resolution;
	gu_boolean colorDevice;
	char *faxSupport;
	char *ttRasterizer;
	sash fonts;
	sash options;
	sash VMOptions;
	};

/* queue information */
struct QUEUE_INFO {
	pool subpool;
	enum QUEUEINFO_TYPE type;
	char *name;
	char *comment;
	gu_boolean transparentMode;
	gu_boolean psPassThru;
	vector printers;
	vector fontlist;		/* fonts held in common */
	};

/* Structure used to describe an *Option entry. */
struct OPTION
	{
	char *name;
	char *value;
	} ;

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

	pip = c2_pmalloc(qip->subpool, sizeof(struct PRINTER_INFO));
	vector_push_back(qip->printers, pip);
	pip->name = pstrdup(qip->subpool, name);
	pip->interface = NULL;
	pip->protocols.TBCP = FALSE;
	pip->protocols.PJL = FALSE;
	pip->feedback = -1;
	pip->jobbreak = JOBBREAK_DEFAULT;
	pip->codes = CODES_DEFAULT;
	pip->binaryOK = FALSE;
	pip->ppdFile = NULL;
	pip->product = NULL;
	pip->psLanguageLevel = 1;
	pip->psVersionStr = NULL;
	pip->psVersion = 0.00;
	pip->psRevision = 0;
	pip->psFreeVM = 0;
	pip->resolution = NULL;
	pip->colorDevice = FALSE;
	pip->faxSupport = NULL;
	pip->ttRasterizer = NULL;
	pip->fonts = new_sash(qip->subpool);
	pip->options = new_sash(qip->subpool);
	pip->VMOptions = new_sash(qip->subpool);

	while((line = gu_getline(line, &line_available, conf)))
		{
		DODEBUG(("line: %s", line));
		switch(line[0])
			{
			case 'C':
				if((p = lmatchp(line, "Comment:")) && depth == 0)
					{
					qip->comment = pstrdup(qip->subpool, p);
					continue;
					}
				if((p = lmatchp(line, "Codes:")))
					{
					pip->codes = atoi(p);
					continue;
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
					pip->interface = pstrdup(qip->subpool, p);
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
					pip->ppdFile = pstrdup(qip->subpool, p);
					continue;
					}
				if((p = lmatchp(line, "PPDOpt:")))
					{
					char *name, *value;
					if((name = gu_strsep(&p," ")) && (value = gu_strsep(&p," ")))
						{
						name = pstrdup(qip->subpool, name);
						value = pstrdup(qip->subpool, value);
						sash_insert(pip->options, name, value);			
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

	if(pip->ppdFile)
		{
		void *ppd = ppdobj_new(pip->ppdFile);

		/* These flags are used to ensure that we heed only the first instance. */
		gu_boolean saw_ColorDevice = FALSE;
		gu_boolean saw_LanguageLevel = FALSE;

		gu_Try {
			char *p;
			while((line = ppdobj_readline(ppd)))
				{
				DODEBUG(("PPD: %s", line));
				if(line[0] == '*')
					{
					switch(line[1])
						{
						case 'C':
							if((p = lmatchp(line, "*ColorDevice:")))
								{
								if(!saw_ColorDevice)
									{
									gu_torf_setBOOL(&pip->colorDevice, p);	/* !!! no error trap !!! */
									saw_ColorDevice = TRUE;
									}
								}
						case 'D':
							if((p = lmatchp(line, "*DefaultResolution:")))
								{
								if(!pip->resolution)
									{
									pip->resolution = pstrdup(qip->subpool, p);
									}
								continue;
								}
						case 'F':
							if((p = lmatchp(line, "*FaxSupport:")))
								{
								if(!pip->faxSupport)
									{
									pip->faxSupport = pstrdup(qip->subpool, p);
									}
								continue;
								}
							if((p = lmatchp(line, "*Font")))
								{
								p = pstrndup(qip->subpool, p, strcspn(p, ":"));
								sash_insert(pip->fonts, p, "");	
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
						case 'P':
							if((p = lmatchp(line, "*Product:")))
								{
								if(*p == '"' && !pip->product)
									{
									pip->product = ppd_finish_QuotedValue(ppd, p+1);
									pool_register_malloc(qip->subpool, pip->product);
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
										pip->psVersionStr = pstrdup(qip->subpool, p);
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
						case 'T':
							if((p = lmatchp(line, "*TTRasterizer:")))
								{
								if(!pip->ttRasterizer)
									{
									pip->ttRasterizer = pstrdup(qip->subpool, p);
									}
								continue;
								}
							break;
						case 'V':
							if((p = lmatchp(line, "*VMOption ")))
								{
								char *name = pstrndup(qip->subpool, p, strcspn(p, "/:"));
								p += strcspn(p, ":");
								if(*p == ':')
									{
									p++;
									p += strspn(p, " \t");
									if(*p == '"')
										{
										p++;
										sash_insert(pip->VMOptions, name, pstrndup(qip->subpool, p, strcspn(p, "\"")));	
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
			if(sash_get(pip->options, "*InstalledMemory", name))
				{
				const char *value_string;
				if(sash_get(pip->VMOptions, name, value_string))
					{
					pip->psFreeVM = atoi(value_string);
					}
				}
			}
			}
		gu_Final {
			ppdobj_delete(ppd);
			}
		gu_Catch {
			gu_ReThrow();
			}
		}

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
					qip->comment = pstrdup(qip->subpool, p);
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
					qip->comment = pstrdup(qip->subpool, p);
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
	struct QUEUE_INFO *qip = gu_alloc(1, sizeof(struct QUEUE_INFO));

	qip->subpool = new_subpool(global_pool);
	qip->type = qit;
	qip->name = pstrdup(qip->subpool, name);
	qip->comment = NULL;
	qip->transparentMode = FALSE;
	qip->psPassThru = FALSE;
	qip->printers = new_vector(qip->subpool, struct PRINTER_INFO*);
	qip->fontlist = new_vector(qip->subpool, char*);

	gu_Try {
		switch(qit)
			{
			case QUEUEINFO_SEARCH:
				if(do_alias(qip))
					qip->type = QUEUEINFO_ALIAS;
				else if(do_group(qip, qip->name, 0))
					qip->type = QUEUEINFO_GROUP;
				else if(do_printer(qip, qip->name, 0))
					qip->type = QUEUEINFO_PRINTER;
				else
					gu_Throw("no alias, group, or printer called \"%s\"", qip->name);
				break;
	
			case QUEUEINFO_ALIAS:
				if(!do_alias(qip))
					gu_Throw("no alias called \"%s\"", qip->name);
				break;
	
			case QUEUEINFO_GROUP:
				if(!do_group(qip, qip->name, 0))
					gu_Throw("no group called \"%s\"", qip->name);
				break;
	
			case QUEUEINFO_PRINTER:
				if(!do_printer(qip, qip->name, 0))
					gu_Throw("no printer called \"%s\"", qip->name);
				break;
			}

		/* Create a font list which includes all fonts which the printers
		 * hold in common. */
		if(vector_size(qip->printers) > 0)
			{
			struct PRINTER_INFO *pip;
			int x, y;
			const char *fontname;
			vector_get(qip->printers, 0, pip);
			vector keys = sash_keys(pip->fonts);
			for(x=0; x < vector_size(keys); x++)
				{
				vector_get(keys, x, fontname);
				for(y=1; y < vector_size(qip->printers); y++)
					{
					vector_get(qip->printers, y, pip);
					if(!sash_exists(pip->fonts, fontname))
						break;
					}
				if(y == vector_size(qip->printers))
					vector_push_back(qip->fontlist, fontname);
				}
			}
	
		}
	gu_Catch {
		queueinfo_delete(qip);
		gu_ReThrow();
		}

	return (void *)qip;
	}

/** discard a queueinfo object
*/
void queueinfo_delete(void *p)
	{
	struct QUEUE_INFO *qip = (struct QUEUE_INFO *)p;
	delete_pool(qip->subpool);
	gu_free(p);
	}

/** return the name of the queue
*/
const char *queueinfo_name(void *p)
	{
	struct QUEUE_INFO *qip = (struct QUEUE_INFO *)p;
	return qip->name;
	}

/** return the description of the queue
*/
const char *queueinfo_comment(void *p)
	{
	struct QUEUE_INFO *qip = (struct QUEUE_INFO *)p;
	return qip->comment;
	}

/** Is the queue in transparent mode?
*/
gu_boolean queueinfo_transparentMode(void *p)
	{
	struct QUEUE_INFO *qip = (struct QUEUE_INFO *)p;
	return qip->transparentMode;
	}

/** Will the queue pass PostScript documents through unchanged?
*/
gu_boolean queueinfo_psPassThru(void *p)
	{
	struct QUEUE_INFO *qip = (struct QUEUE_INFO *)p;
	return qip->psPassThru;
	}

/** Can all possible 8-bit character codes be passed thru to the PS interpreter?
 *
 * We return TRUE if all member printers can pass all 8-bit codes.  If there
 * are no member printers, we return FALSE.
*/
gu_boolean queueinfo_binaryOK(void *p)
	{
	struct QUEUE_INFO *qip = (struct QUEUE_INFO *)p;
	int i;
	gu_boolean answer = FALSE;

	for(i=0; i < vector_size(qip->printers); i++)
		{
		struct PRINTER_INFO *pip;
		vector_get(qip->printers, i, pip);
		if(!pip->binaryOK)
			return FALSE;
		else
			answer = TRUE;
		}

	return answer;
	}

/** read the name of the queue's printer(s)'s PPD file(s)
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

	for(i=0; i < vector_size(qip->printers); i++)
		{
		vector_get(qip->printers, i, pip);
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

	for(i=0; i < vector_size(qip->printers); i++)
		{
		vector_get(qip->printers, i, pip);
		if(!pip->product)
			return NULL;
		if(!answer)
			answer = pip->product;
		else if(strcmp(answer, pip->product))
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

	for(i=0; i < vector_size(qip->printers); i++)
		{
		vector_get(qip->printers, i, pip);
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
 * available.
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

	for(i=0; i < vector_size(qip->printers); i++)
		{
		vector_get(qip->printers, i, pip);
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

	for(i=0; i < vector_size(qip->printers); i++)
		{
		vector_get(qip->printers, i, pip);
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

	for(i=0; i < vector_size(qip->printers); i++)
		{
		struct PRINTER_INFO *pip;
		vector_get(qip->printers, i, pip);
		if(!pip->binaryOK)
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

	for(i=0; i < vector_size(qip->printers); i++)
		{
		vector_get(qip->printers, i, pip);
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

	for(i=0; i < vector_size(qip->printers); i++)
		{
		vector_get(qip->printers, i, pip);
		if(!pip->ttRasterizer)
			return NULL;
		else if(!answer)
			answer = pip->ttRasterizer;
		else if(strcmp(answer, pip->ttRasterizer))
			return NULL;
		}

	return answer;
	}

/** How many fonts are in the font list?
*/
int queueinfo_fontCount(void *p)
	{
	struct QUEUE_INFO *qip = (struct QUEUE_INFO *)p;
	return vector_size(qip->fontlist);
	}

/** Return item index from the printer's font list
*/
const char *queueinfo_font(void *p, int index)
	{
	struct QUEUE_INFO *qip = (struct QUEUE_INFO *)p;
	const char *fontname;
	if(index > vector_size(qip->fontlist))
		return NULL;
	vector_get(qip->fontlist, index, fontname);
	return fontname;
	}

/** Check if a specified font exists
*/
gu_boolean queueinfo_fontExists(void *p, const char name[])
	{
	struct QUEUE_INFO *qip = (struct QUEUE_INFO *)p;
	struct PRINTER_INFO *pip;
	int i;
	for(i=0; i < vector_size(qip->printers); i++)
		{
		vector_get(qip->printers, i, pip);
		if(!sash_exists(pip->fonts, name))
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
	const char *value;
	const char *answer = NULL;
	if(!queueinfo_ppdFile(p))
		return NULL;
	for(i=0; i < vector_size(qip->printers); i++)
		{
		vector_get(qip->printers, i, pip);
		if(!sash_get(pip->options, name, value))
			return FALSE;
		if(!answer)
			answer = value;
		else if(strcmp(answer, value))
			return NULL;
		}
	return answer;
	}

/*
** Test program
** gcc -Wall -DTEST -I../include queueinfo.c ../libppr.a ../libgu.a -lz
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
	obj = queueinfo_new(QUEUEINFO_SEARCH, argv[1]);
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
	return 0;
	}
#endif

/* end of file */
