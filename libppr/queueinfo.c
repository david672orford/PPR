/*
** mouse:~ppr/src/libppr/queueinfo.c
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
** Last modified 31 December 2003.
*/

/*+ \file

This module contains the implementation of an object which describes a PPR queue.  An
instance may be created and automatically populated with the attributes of a specific
PPR queue.

*/

#include "before_system.h"
#include <stdio.h>
#include <string.h>
#include "gu.h"
#include "global_defines.h"
#include "queueinfo.h"

struct QI {
	enum QUEUEINFO_TYPE type;
	void *name;
	void *comment;
	void *ppdfile;
	gu_boolean transparent_mode;

	};

static void do_printer(struct QI *qip, const char name[])
	{
	char fname[MAX_PPR_PATH];
	FILE *pconf;
	char *line = NULL;
	int line_available = 80;
	char *p;

	ppr_fnamef(fname, "%s/%s", PRCONF, name);
	if(!(pconf = fopen(fname, "r")))
		libppr_throw(EXCEPTION_MISSING, "do_printer", "file not found");

	while((line = gu_getline(line, &line_available, pconf)))
		{
		switch(line[0])
			{
			case 'C':
				if((p = lmatchp(line, "Comment:")))
					{
					
					continue;
					}
				break;
			case 'P':
				if((p = lmatchp(line, "PPDFile:")))
					{

					continue;
					}
				if((p = lmatchp(line, "PPDOpt:")))
					{
			
					continue;
					}
				break;
			}

		}

	fclose(pconf);
	}

/** create a queueinfo object

*/
void *queueinfo_new(enum QUEUEINFO_TYPE qit, const char name[])
	{
	struct QI *qip = gu_alloc(1, sizeof(struct QI));
	
	qip->type = qit;
	qip->name = gu_pcs_new_cstr(name);
	qip->name = gu_pcs_new();
	qip->comment = gu_pcs_new();
	qip->ppdfile = gu_pcs_new();
	qip->transparent_mode = FALSE;

	if(qit == QUEUEINFO_ALIAS)
		{
		do_alias(qip, name);
		}

	if(qit == QUEUEINFO_GROUP)
		{
		do_group(qip, name);
		}

	else if(qit == QUEUEINFO_PRINTER)
		{
		do_printer(qip, name);
		}

	else
		{
		libppr_throw(EXCEPTION_BADUSAGE, "queueinfo_new", "unknown queue type");
		}

	return (void *)qip;
	}

/** discard a queueinfo object
*/
void queueinfo_delete(void *p)
	{
	gu_free(p);
	}

/**
*/
const char *queueinfo_product(void *p)
	{
	struct QI *qip = (struct QI *)p;

	}

/**
*/
int queueinfo_psLanguageLevel(void *p)
	{
	struct QI *qip = (struct QI *)p;

	}

/**
*/
double queueinfo_psVersion(void *p)
	{
	struct QI *qip = (struct QI *)p;

	}

/**
*/
int queueinfo_psRevision(void *p)
	{
	struct QI *qip = (struct QI *)p;

	}

/**
*/
int queueinfo_psFreeVm(void *p)
	{
	struct QI *qip = (struct QI *)p;

	}

/**
*/
const char *queueinfo_resolution(void *p)
	{
	struct QI *qip = (struct QI *)p;

	}

/**
*/
gu_boolean queueinfo_colorDevice(void *p)
	{
	struct QI *qip = (struct QI *)p;

	}

/**
*/
const char *queueinfo_faxSupport(void *p)
	{
	struct QI *qip = (struct QI *)p;

	}

/**
*/
const char *queueinfo_ttRasterizer(void *p)
	{
	struct QI *qip = (struct QI *)p;

	}

/**
*/
int *queueinfo_fontCount(void *p)
	{
	struct QI *qip = (struct QI *)p;

	}

/**
*/
const char *queueinfo_font(void *p, int index)
	{
	struct QI *qip = (struct QI *)p;

	}

/**
*/
gu_boolean queueinfo_fontExists(void *p, const char name[])
	{
	struct QI *qip = (struct QI *)p;

	}

/**
*/
const char *queueinfo_optionValue(void *p, const char name[])
	{
	struct QI *qip = (struct QI *)p;

	}

/* end of file */
