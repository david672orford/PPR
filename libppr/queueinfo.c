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
** Last modified 22 January 2004.
*/

/*+ \file

This module contains the implementation of an object which describes a PPR queue.  An
instance may be created and automatically populated with the attributes of a specific
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

/* printer information */
struct PRINTER_INFO {
	gu_boolean binaryOK;
	char *ppdFile;
	char *product;
	int psLanguageLevel;
	char *psVersionStr;
	double psVersion;
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
	vector printers;
	};

/* Structure used to describe an *Option entry. */
struct OPTION
	{
	char *name;
	char *value;
	} ;

static void do_printer(struct QUEUE_INFO *qip, const char name[], int depth)
	{
	char fname[MAX_PPR_PATH];
	FILE *conf;
	char *line = NULL;
	int line_available = 80;
	char *p;

	ppr_fnamef(fname, "%s/%s", PRCONF, name);
	if(!(conf = fopen(fname, "r")))
		gu_Throw("file not found");

	struct PRINTER_INFO *pip = c2_pmalloc(qip->subpool, sizeof(struct PRINTER_INFO));
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
				break;
			}
		}

	fclose(conf);

	if(pip->ppdFile)
		{
		void *ppd = ppdobj_new(pip->ppdFile);
		gu_Try {
			char *p;
			while((p = ppdobj_readline(ppd)))
				{
				}
			}
		gu_Final {
			ppdobj_delete(ppd);
			}
		gu_Catch {
			gu_ReThrow();
			}
		}
	}

static void do_group(struct QUEUE_INFO *qip, const char name[], int depth)
	{
	char fname[MAX_PPR_PATH];
	FILE *conf;
	char *line = NULL;
	int line_available = 80;
	char *p;

	ppr_fnamef(fname, "%s/%s", PRCONF, name);
	if(!(conf = fopen(fname, "r")))
		gu_Throw("file not found");

	while((line = gu_getline(line, &line_available, conf)))
		{
		switch(line[0])
			{
			case 'C':
				if((p = lmatchp(line, "Comment:")) && depth == 0)
					{
					qip->comment = pstrdup(qip->subpool, p);
					continue;
					}
				break;
			case 'M':
				if((p = lmatchp(line, "Member:")))
					{
					do_printer(qip, p, depth + 1);
					}
				break;
			}
		}

	fclose(conf);
	}

static void do_alias(struct QUEUE_INFO *qip, const char name[])
	{
	char fname[MAX_PPR_PATH];
	FILE *conf;
	char *line = NULL;
	int line_available = 80;
	char *p;

	ppr_fnamef(fname, "%s/%s", PRCONF, name);
	if(!(conf = fopen(fname, "r")))
		gu_Throw("file not found");

	while((line = gu_getline(line, &line_available, conf)))
		{
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
					char test_fname[MAX_PPR_PATH];
					struct stat statbuf;
					ppr_fnamef(test_fname, "%s/%s", GRCONF, name);
					if(stat(test_fname, &statbuf) == 0)
						do_group(qip, p, 1);
					else
						do_printer(qip, p, 1);
					continue;
					}
				break;
			}
		}

	fclose(conf);
	}

/** create a queueinfo object
*/
void *queueinfo_new(enum QUEUEINFO_TYPE qit, const char name[])
	{
	struct QUEUE_INFO *qip = gu_alloc(1, sizeof(struct QUEUE_INFO));
	qip->subpool = new_subpool(global_pool);
	
	qip->type = qit;
	qip->name = gu_pcs_new_cstr(name);
	qip->comment = NULL;
	qip->transparentMode = FALSE;

	switch(qit)
		{
		case QUEUEINFO_ALIAS:
			do_alias(qip, name);
			break;

		case QUEUEINFO_GROUP:
			do_group(qip, name, 0);
			break;

		case QUEUEINFO_PRINTER:
			do_printer(qip, name, 0);
			break;
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

/** read the name of the queue
*/
const char *queueinfo_name(void *p)
	{
	struct QUEUE_INFO *qip = (struct QUEUE_INFO *)p;
	return qip->name;
	}

/** read the description of the queue
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

#if 0

/** can any character code be passed thru to the PS interpreter?
*/
gu_boolean queueinfo_binaryOK(void *p)
	{
	struct QUEUE_INFO *qip = (struct QUEUE_INFO *)p;
	return qip->binaryOK;
	}

/** read the name of the queue's printer(s)'s PPD file(s)
*/
const char *queueifno_ppdFile(void *p)
	{
	struct QUEUE_INFO *qip = (struct QUEUE_INFO *)p;
	return qip->ppdFile;
	}

/** read the PostScript product string of the queue's printer(s)
*/
const char *queueinfo_product(void *p)
	{
	struct QUEUE_INFO *qip = (struct QUEUE_INFO *)p;
	return qip->product;
	}

/** read the PostScript language level of the queue's printer(s)
*/
int queueinfo_psLanguageLevel(void *p)
	{
	struct QUEUE_INFO *qip = (struct QUEUE_INFO *)p;
	return qip->psLanguageLevel;
	}

/** What is the version string of the PostScript interpreter?
*/
const char *queueinfo_psVersionStr(void *p)
	{
	struct QUEUE_INFO *qip = (struct QUEUE_INFO *)p;
	return qip->psVersionStr;
	}

/** What is the version number of the PostScript interpreter?
*/
double queueinfo_psVersion(void *p)
	{
	struct QUEUE_INFO *qip = (struct QUEUE_INFO *)p;
	return qip->psVersion;
	}

/** What is the revision number of the printer-specific parts of the PostScript interpreter?
*/
int queueinfo_psRevision(void *p)
	{
	struct QUEUE_INFO *qip = (struct QUEUE_INFO *)p;
	return qip->psRevision;
	}

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

/* end of file */
