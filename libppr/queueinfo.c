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
** Last modified 27 January 2004.
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
	pip->binaryOK = TRUE;
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
								printf("y\n");
								if(*p == '"' && !pip->product)
									{
									pip->product = ppd_finish_QuotedValue(ppd, p+1);
									pool_register_malloc(qip->subpool, pip->product);
									}
								continue;
								}
							if((p = lmatchp(line, "*PSVersion:")))
								{
								printf("*****\n");
								if(*p == '"' && !pip->psVersionStr)
									{
									float version;
									int revision;
									p++;
									p[strcspn(p, "\"")] = '\0';
									if(gu_sscanf(p, "(%f) %d", &version, &revision) == 2)
										{
											printf("***************\n");
										pip->psVersionStr = pstrdup(qip->subpool, p);
										pip->psVersion = version;
										pip->psRevision = revision;
										}
									}
								continue;
								}
							break;
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

/** is the queue in transparent mode?
*/
gu_boolean queueinfo_transparentMode(void *p)
	{
	struct QUEUE_INFO *qip = (struct QUEUE_INFO *)p;
	return qip->transparentMode;
	}

/** will the queue pass PostScript documents through unchanged?
*/
gu_boolean queueinfo_psPassThru(void *p)
	{
	struct QUEUE_INFO *qip = (struct QUEUE_INFO *)p;
	return qip->psPassThru;
	}

/** can all possible 8-bit character codes be passed thru to the PS interpreter?
 *
 * We return true if all member printers can pass all 8-bit codes.
*/
gu_boolean queueinfo_binaryOK(void *p)
	{
	struct QUEUE_INFO *qip = (struct QUEUE_INFO *)p;
	int i;
	gu_boolean answer = TRUE;

	for(i=0; i < vector_size(qip->printers); i++)
		{
		struct PRINTER_INFO *pip;
		vector_get(qip->printers, i, pip);
		if(!pip->binaryOK)
			{
			answer = FALSE;
			break;
			}
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
		if(!answer)
			{
			answer = pip->ppdFile;
			}
		else if(strcmp(answer, pip->ppdFile))
			{
			answer = NULL;
			break;
			}
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
		if(!answer)
			{
			answer = pip->product;
			}
		else if(strcmp(answer, pip->product))
			{
			answer = NULL;
			break;
			}
		}

	return answer;
	}

/*
 * Find the member with the oldest interpreter.
 */
static struct PRINTER_INFO *find_lowest_version(struct QUEUE_INFO *qip)
	{
	int i;
	struct PRINTER_INFO *pip, *lowest = NULL;

	for(i=0; i < vector_size(qip->printers); i++)
		{
		vector_get(qip->printers, i, pip);
		if(!lowest || pip->psVersion < lowest->psVersion || (pip->psVersion == lowest->psVersion && pip->psRevision < lowest->psRevision))
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
		return 1;
	}

/** What is the version string of the PostScript interpreter?
*/
const char *queueinfo_psVersionStr(void *p)
	{
	struct QUEUE_INFO *qip = (struct QUEUE_INFO *)p;
	struct PRINTER_INFO *pip = find_lowest_version(qip);
	if(pip)
		return pip->psVersionStr;
	else
		return "(47.0) 0";
	}

/** What is the version number of the PostScript interpreter?
*/
double queueinfo_psVersion(void *p)
	{
	struct QUEUE_INFO *qip = (struct QUEUE_INFO *)p;
	struct PRINTER_INFO *pip = find_lowest_version(qip);
	if(pip)
		return pip->psVersion;
	else
		return 47.0;
	}

/** What is the revision number of the printer-specific parts of the PostScript interpreter?
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

#if 0
/** How many bytes of PostScript memory are available to programs?
*/
int queueinfo_psFreeVM(void *p)
	{
	struct QUEUE_INFO *qip = (struct QUEUE_INFO *)p;
	return qip->psFreeVM;
	}

/** What is the resolution (expressed as a string)?
*/
const char *queueinfo_resolution(void *p)
	{
	struct QUEUE_INFO *qip = (struct QUEUE_INFO *)p;
	return qip->resolution;
	}

/** Can the printer print in color?
*/
gu_boolean queueinfo_colorDevice(void *p)
	{
	struct QUEUE_INFO *qip = (struct QUEUE_INFO *)p;
	return qip->colorDevice;
	}

/** What kind of fax support does the printer have, if any?
*/
const char *queueinfo_faxSupport(void *p)
	{
	struct QUEUE_INFO *qip = (struct QUEUE_INFO *)p;
	return qip->faxSupport;
	}

/** What kind of TrueType rasterizer does the printer have, if any?
*/
const char *queueinfo_ttRasterizer(void *p)
	{
	struct QUEUE_INFO *qip = (struct QUEUE_INFO *)p;
	return qip->ttRasterizer;
	}

/** how many fonts are in the font list?
*/
int queueinfo_fontCount(void *p)
	{
	struct QUEUE_INFO *qip = (struct QUEUE_INFO *)p;
	return qip->fontCount;
	}

/** return item index from the printer's font list
*/
const char *queueinfo_font(void *p, int index)
	{
	struct QUEUE_INFO *qip = (struct QUEUE_INFO *)p;
	return qip->fonts[index];	
	}

/** check if a specified font exists
*/
gu_boolean queueinfo_fontExists(void *p, const char name[])
	{
	struct QUEUE_INFO *qip = (struct QUEUE_INFO *)p;
	return FALSE;
	}

/**
*/
const char *queueinfo_optionValue(void *p, const char name[])
	{
	struct QUEUE_INFO *qip = (struct QUEUE_INFO *)p;
	return NULL;
	}

#endif

/*
** Test program
** gcc -Wall -DTEST -I../include queueinfo.c ../libppr.a ../libgu.a -lz
*/
#ifdef TEST
int main(int argc, char *argv[])
	{
	void *obj;
	const char *p;
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
	printf("psVersion[] = \"%f\"\n", queueinfo_psVersion(obj));	
	printf("psRevision[] = \"%d\"\n", queueinfo_psRevision(obj));	
	return 0;
	}
#endif

/* end of file */
