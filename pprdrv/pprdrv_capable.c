/*
** mouse:~ppr/src/pprdrv/pprdrv_capable.c
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
** Last modified 6 February 2003.
*/

/*===========================================================================
** Determine if the printer is capable of printing the job.  If the printer 
** is not capable, then message are added to the job log and the job will be
** turned away from this printer.
**
** Resource substitution descisions are also made in this module.
===========================================================================*/

#include "before_system.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "global_structs.h"
#include "interface.h"
#include "pprdrv.h"

/* This defines the resource search path when --cache-priority=low. */
static const enum RES_SEARCH cache_priority_low_search_list[] =
	{
	RES_SEARCH_FONTINDEX,
	RES_SEARCH_CACHE,
	RES_SEARCH_END
	};

/* This defines the resource search path when --cache-priority=high. */
static const enum RES_SEARCH cache_priority_high_search_list[] =
	{
	RES_SEARCH_CACHE,
	RES_SEARCH_FONTINDEX,
	RES_SEARCH_END
	};

/* These are the resources that PostScript printers have built in that
   are not mentioned in the PPD files. */
static struct INTERNAL_RESOURCES
	{
	const char *type;
	const char *name;
	int minlevel;
	} internal_resources[] =
	{
	{"encoding", "StandardEncoding", 1},
	{"encoding", "SymbolEncoding", 1},
	{"encoding", "ISOLatin1Encoding", 2},
	{"procset", "CIDInit", 3},					/* ??? is this true ??? */
	{NULL, NULL, 0}
	};

/*===========================================================================
** These functions are used by check_if_capable() to add messages to the
** job's log file.  These functions call the job log functions in 
** pprdrv_log.c to do the actual logging.  These functions just provide 
** a way to add extra lines that mark off the start of a report on 
** the incapabability of a printer.
===========================================================================*/

static gu_boolean log_started = FALSE;
static gu_boolean log_said_incapable = FALSE;
static gu_boolean log_said_willtry = FALSE;

/*
** All of the functions that log incapabilities call this first to make sure
** that the proper heading has already been printed.
*/
static void start_incapable_log(int group_pass)
	{
	if(!log_started)
		{
		/* If there may be 2 passes, say which one this is. */
		if(group_pass)
			{
			if(group_pass == 1)
				log_printf("+Pass 1\n");
			else
				log_printf("+Pass %d, ProofMode=%s\n", group_pass, job.attr.proofmode == PROOFMODE_TRUSTME ? "TrustMe" : job.attr.proofmode == PROOFMODE_SUBSTITUTE ? "Substitute" : "NotifyMe");
			}
		}
	} /* end of start_incapable_log() */

/*
** This function logs a reason we will not try to print the job on this printer.
*/
static void incapable_log(int group_pass, const char *s, ...)
	{
	va_list va;

	start_incapable_log(group_pass);

	/*
	** If we have not previously said that this printer
	** is incapable, say so now.
	*/
	if(! log_said_incapable)
		{
		log_printf(_("+Job turned away from printer \"%s\" for the following reason(s):\n"), printer.Name);
		log_said_incapable = TRUE;
		}

	log_putc('+');				/* begin with a plus */
	va_start(va, s);
	log_vprintf(s, va);			/* print the error message */
	va_end(va);
	log_putc('\n');				/* add a newline */
	} /* end of incapable_log() */

/*
** This function logs a reason why we think the job may not print correctly on this
** printer even though we are going to try.
*/
static void incapable_log_willtry(int group_pass, const char *s, ...)
	{
	va_list va;

	start_incapable_log(group_pass);

	/*
	** If we have not yet said that we are going to try to print even
	** though all requirements are not perfectly met, and we have
	** not said that we won't try to print, then say now that
	** we will do our best.
	*/
	if(!log_said_willtry && !log_said_incapable)
		{
		log_printf(_("+Printer \"%s\" may not print this job correctly for the following reason(s):\n"), printer.Name);
		log_said_willtry = TRUE;
		}

	log_putc('+');				/* begin with a plus */
	va_start(va, s);
	log_vprintf(s, va);			/* print the error message */
	va_end(va);
	log_putc('\n');				/* add a newline */
	} /* end of incapable_log_willtry() */

/*
** This function doesn't really do anything important, at least at the moment.
*/
static void end_of_incapable_log(void)
	{
	if(log_started)
		{
		log_started = log_said_incapable = log_said_willtry = FALSE;
		}
	} /* end of end_of_incapable_log() */

/*===================================================================
** Here is the giant function which makes this module famous!
**
** Read the required resources and requirements from
** the open queue file, and return -1 if they can not
** all be met satisfactorily.
**
** Also, check some of the things stored in the "Attr:" line.
===================================================================*/
int check_if_capable(FILE *qfile, int group_pass)
	{
	const char *function = "check_if_capable";
	int incapable = FALSE;				/* final result of this function */
	int incapable_fonts = 0;			/* number of missing fonts (not substituted) */
	int incapable_resources = 0;		/* number of other missing resources */
	int incapable_requirements = 0;		/* missing requirements */
	int incapable_prohibitions = 0;		/* GrayOK = FALSE */
	int incapable_codes = 0;
	int incapable_pageslimit = 0;
	int incapable_kilobyteslimit = 0;
	int color_document = FALSE;

	char *qline = NULL;					/* for reading lines from the queue file */
	int qline_available = 80;

	char *f1, *f2, *f3, *f4, *f5, *f6;

	DODEBUG_RESOURCES(("%s()", function));

	/*-----------------------------------------------------------------
	** Check for the resources that are required to print this job.
	** Read the "Res: " lines from the queue file.
	-----------------------------------------------------------------*/
	{
	struct DRVRES *d;			/* pointer to next resource structure */
	int font;					/* TRUE if current resource is a font. */
	const char *fnptr;			/* Temporary pointer to file name. */
	int features;
	const enum RES_SEARCH *search_list;

	search_list = job.CachePriority == CACHE_PRIORITY_HIGH
				? cache_priority_high_search_list
				: cache_priority_low_search_list;

	while((qline = gu_getline(qline, &qline_available, qfile)))
		{										/* Work until end of file */
		DODEBUG_RESOURCES(("%s(): line: %s", function, qline));

		if(strcmp(qline, "EndRes") == 0)
			break;								/* or better yet, an "EndRes" line. */

		if(strncmp(qline, "Res:", 4))
			fatal(EXIT_JOBERR, "Queue file does not begin with \"Res:\": %s", qline);

		{
		char *p = qline;
		if(!gu_strsep(&p, " ")
				|| !(f1 = gu_strsep(&p, " "))
				|| !(f2 = gu_strsep(&p, " "))
				|| !(f3 = gu_strsep(&p, " "))
				|| !(f4 = gu_strsep_quoted(&p, " ", NULL))
				|| !(f5 = gu_strsep(&p, " "))
				|| !(f6 = gu_strsep(&p, " ")))
			fatal(EXIT_JOBERR, "Queue file line has too few arguments: %s", qline);
		}

		/* Make a new DRVRES record. */
		d = add_drvres(atoi(f1), atoi(f2), gu_strdup(f3), gu_strdup(f4), gu_getdouble(f5), atoi(f6));

		if(! d->needed)							/* ignore those */
			continue;							/* not still ``needed'' */

		DODEBUG_RESOURCES(("Need resource %s %s", d->type, d->name));

		/*
		** If it is a font, set a flag.  If it is a font,
		** try to find it in the printer's PPD file.
		*/
		font = FALSE;
		if(strcmp(d->type, "font") == 0)		/* If it is a font, */
			{									/* set a flag for */
			font = TRUE;						/* future reference. */
			if(ppd_font_present(d->name))		/* If it is in the printer, */
				{
				DODEBUG_RESOURCES(("Font %s is in printer", d->name));
				continue;						/* then be satisfied. */
				}
			}

		/*
		** Don't bother looking for encodings which are built
		** into the printer.  For level 1 printers these are
		** "Standard" and "Symbol", for level 2 printers, "ISOLatin1"
		** is also built in.
		*/
		{
		int i;
		for(i=0; internal_resources[i].type; i++)
			{
			if(strcmp(d->type, internal_resources[i].type) == 0
						&& strcmp(d->name, internal_resources[i].name) == 0
						&& Features.LanguageLevel >= internal_resources[i].minlevel)
				{
				DODEBUG_RESOURCES(("Resource %s %s is in printer", d->type, d->name));
				break;
				}
			}
		if(internal_resources[i].type)			/* if didn't reach end of table, */
			continue;
		}

		/*
		** See if the resource in question is in the cache.
		*/
		if((fnptr = find_cached_resource(d->type, d->name, d->version, d->revision, search_list, (int*)NULL, &features, NULL)))
			{									/* If found, */
			DODEBUG_RESOURCES(("resource %s %s is in cache file \%s\"", d->type, d->name, fnptr));

			if(features & FONT_TYPE_42)			/* if it has a type 42 section, */
				{
				DODEBUG_RESOURCES(("(It is a font with a type 42 section.)"));

				want_ttrasterizer();			/* a rasterizer, even if we have to load one. */

				/* If rasterizer load failed and there is no Type1 section, */
				if( !printer.type42_ok && !(features & FONT_TYPE_1) )
					{
					incapable_log(group_pass, "Font \"%s\" has no Type1 section and printer has no TT rasterizer", d->name);
					incapable = TRUE;
					continue;
					}
				}

			else if(features & FONT_TYPE_TTF)	/* if it is a .TTF file, */
				{
				DODEBUG_RESOURCES(("(It is a TrueType font in .TTF format.)"));

				/* Try for a TrueType rasterizer so we won't have to use Type 3. */
				want_ttrasterizer();

				d->dot_ttf = TRUE;
				}

			d->filename = fnptr;				/* remember the name of the cache file */
			d->needed = FALSE;					/* resource is now a supplied resource */
			if(features & FONT_MACTRUETYPE) d->mactt = TRUE;

			continue;							/* Go on to next resource. */
			}

		/*
		** Here is where we do font substitution, but
		** we only do it if the ProofMode is "Substitute".
		*/
		if(font && job.attr.proofmode==PROOFMODE_SUBSTITUTE)
			{
			FILE *subfile;
			char temp[80];				/* subfile lines won't be very long */
			int in_entry=FALSE;			/* TRUE while we process the entry we are looking for. */
			char *ptr;
			char *codeptr;
			int done=FALSE;

			/* If we can find a sub file, try it. */
			if((subfile = fopen(FONTSUB, "r")))
				{
				while(!done && fgets(temp,sizeof(temp),subfile))
					{
					if(temp[0]==';' || temp[0]=='#')			/* ignore comments */
						continue;
					if(temp[strspn(temp," \t\n")] == '\0')		/* ignore blank lines */
						continue;

					if( ! in_entry )							/* if not yet in section for font */
						{										/* for which we want a substitute, */
						if(! isspace(temp[0]))					/* if this is a section heading (section headings do not begin with space) */
							{
							temp[strcspn(temp," \t\n")] = '\0'; /* remove trailing whitespace */
							if(strcmp(d->name,temp)==0)			/* if match */
								in_entry=TRUE;					/* then we found our font entry */
							}
						continue;
						}
					else								/* in entry */
						{
						if(! isspace(temp[0]))			/* start of new entry means */
							break;						/* we should give up */

						ptr=&temp[strspn(temp," \t")];			/* eat leading blank space */
						codeptr=&ptr[strcspn(ptr," \t\n")];		/* point to trailing space */
						*codeptr = '\0';						/* eat trailing space */
						codeptr++;								/* move past NULL */
						codeptr = &codeptr[strspn(codeptr," \t\n")];	/* eat next leading space */
						codeptr[strcspn(codeptr,"\n")] = '\0';	/* delete trailing line feed */

						/*
						** If the substitute font is found, in the printer
						** or in the font cache, set flags to use it instead.
						*/
						fnptr = (char*)NULL;			/* cached PostScript font name pointer */
						if(ppd_font_present(ptr)
								|| (fnptr = find_cached_resource("font", ptr, 0.0, 0, search_list, (int*)NULL, &features, NULL)))
							{
							/* If it must be downloaded from a file, */
							if(fnptr)
								{
								if(features & FONT_TYPE_42)		/* if it has a Type42 section, */
									{
									want_ttrasterizer();		/* try to get a rasterizer */

									/* If rasterizer load failed and there is no Type1 section, */
									if( !printer.type42_ok && !(features & FONT_TYPE_1) )
										{
										incapable_log_willtry(group_pass, _("Can't substitute \"%s\" for \"%s\" because it has no Type1 section."), ptr, d->name);
										continue;				/* try next substitute font */
										}
									}
								else if(features & FONT_TYPE_TTF)
									{
									want_ttrasterizer();
									d->dot_ttf = TRUE;
									}

								d->filename = fnptr;
								if(features & FONT_MACTRUETYPE) d->mactt = TRUE;
								}

							d->former_name = d->name;			/* save the old name */
							d->name = gu_strdup(ptr);			/* substitute the new */
							d->needed = FALSE;					/* no longer need but rather supplied */
							if(*codeptr)						/* if substitution code, save it */
								d->subst_code = gu_strdup(codeptr);
							DODEBUG_RESOURCES(("Substituted font %s for %s", d->name, d->former_name));
							incapable_log_willtry(group_pass, "Font \"%s\" substituted for \"%s\"", d->name, d->former_name);
							done = TRUE;
							}
						}
					}
				fclose(subfile);

				if(done)				/* If we had a match, don't let loop */
					continue;			/* pass this point. */
				}
			} /* end of font substitution */

		/* Add to count of missing resources. */
		if(font)
			incapable_fonts++;
		else
			incapable_resources++;

		/*
		** Here is where we decide if we can go ahead without this
		** resource.  If the proofmode is "NotifyMe", we can't.
		**
		** If the ProofMode is "TrustMe" it ought to be ok to go ahead.
		**
		** If this is a font and the ProofMode is "Substitute" it ought to
		** be ok, it will be replaced with Courier.
		**
		** If this is a group job and we are still on pass 1, don't go ahead
		** without anything, some other printer might have it.
		*/
		if(job.attr.proofmode != PROOFMODE_NOTIFYME
				&& (job.attr.proofmode==PROOFMODE_TRUSTME || font)
				&& group_pass != 1)
			{
			if(strcmp(d->type,"procset")==0)
				incapable_log_willtry(group_pass, _("Resource \"%s %s %s %d\" not available."), d->type, d->name,gu_dtostr(d->version),d->revision);
			else
				incapable_log_willtry(group_pass, _("Resource \"%s %s\" not available."), d->type, d->name);
			continue;
			}

		/*
		** Put the resource in the incapable log and mark the
		** printer as incapable.
		*/
		if(strcmp(d->type,"procset")==0)
			incapable_log(group_pass, _("Resource \"%s %s %s %d\" not available."), d->type, d->name, gu_dtostr(d->version), d->revision);
		else
			incapable_log(group_pass, _("Resource \"%s %s\" not available."), d->type, d->name);

		/* If we get this far, the printer is incapable. */
		incapable = TRUE;				/* printer is incapable */
		} /* end of while loop */
	}

	/*------------------------------------------------------------
	** Requirements as specified in a "%%Requirements:" comment.
	------------------------------------------------------------*/
	while((qline = gu_getline(qline, &qline_available, qfile)))
		{
		DODEBUG_RESOURCES(("%s(): line: %s", function, qline));

		if(strcmp(qline, "EndReq") == 0)		/* read requirements */
			break;								/* until end flag */

		if(drvreq_count >= MAX_DRVREQ)
			fatal(EXIT_JOBERR, "internal pprdrv error: DRVREQ overflow");

		if(strncmp(qline, "Req:", 4))
			fatal(EXIT_JOBERR, "Queue file line does not begin with \"Req:\": %s", qline);

		{
		char *p = qline;
		if(!gu_strsep(&p, " ") || !(f1 = gu_strsep(&p, " ")))
			fatal(EXIT_JOBERR, "Queue file line has too few arguments: %s", qline);

		drvreq[drvreq_count++] = gu_strdup(f1);
		}

		/*
		** In this switch, if the requirement is unrecognized or
		** is satisfied, execute continue.  If we allow execution
		** to drop out of the bottom of this loop, it is understood
		** that the requirement in question has not been met.
		*/
		switch(f1[0])					/* treat first character as a hash value */
			{
			case 'c':					/* requirements begining with 'c' */
				if(strcmp(f1, "color") == 0)
					{
					color_document = TRUE;

					if(!Features.ColorDevice)
						break;
					}
				continue;

			case 'd':					/* requirements begining with 'd' */
				/*
				** This method of checking for duplex support is not
				** the best one possible.  It is possible that duplex
				** is an optional feature and consequently the PPD file
				** will contain duplex code with the stipulation that
				** may only be used if the duplex option is installed.
				*/
				if(strcmp(f1, "duplex") == 0)
					{
					if(!find_feature("*Duplex", "None"))
						break;
					}
				else if(strcmp(f1, "duplex(tumble)") == 0)
					{
					if(!find_feature("*Duplex", "DuplexTumble"))
						break;
					}
				continue;

			case 'f':					/* requirements begining with 'f' */
				if(strcmp(f1, "fax") == 0)
					{
					if(Features.FaxSupport == FAXSUPPORT_NONE)
						break;
					}
				continue;

			default:					/* ignore unrecognized */
				continue;				/* requirements */
			} /* end of switch */

		/*
		** If we get this far, the requirement can't be met.
		** Increment the count of requirments we can't meet.
		*/
		incapable_requirements++;

		/*
		** If the ProofMode is NotifyMe, then the unsatisified requirement
		** or requirements is a reason we can't print this
		** job.  If this is pass 1 on a group, that is a reason
		** we can't print it yet.
		*/
		if(job.attr.proofmode == PROOFMODE_NOTIFYME || group_pass == 1)
			{
			incapable_log(group_pass, _("Requirement \"%s\" can't be met."), f1);
			incapable = TRUE;
			}
		else
			{
			incapable_log_willtry(group_pass, _("Requirement \"%s\" can't be met."), f1);
			}
		} /* end of while loop */

	/*--------------------------------------------------------
	** Check to see if the printer language level
	** is high enough for the job.
	--------------------------------------------------------*/
	if(job.attr.langlevel > Features.LanguageLevel)
		{
		incapable_log(group_pass, _("Required language level (%d) exceeds printer's language level (%d)."), job.attr.langlevel, Features.LanguageLevel);
		incapable = TRUE;
		incapable_requirements++;
		}

	/*--------------------------------------------------------
	** If the printer is not a level two printer,
	** make sure it has the extensions needed to
	** print this job.
	--------------------------------------------------------*/
	if(Features.LanguageLevel < 2)
		{
		if( job.attr.extensions & (~Features.Extensions) )
			{
			incapable_log(group_pass, _("Printer does not have all required extensions."));
			incapable = TRUE;
			incapable_requirements++;
			}
		}

	/*---------------------------------------------------------------
	** If this is not a colour document and the printer is not
	** allowed to print grayscale documents, don't print this job.
	---------------------------------------------------------------*/
	if( !color_document && !printer.GrayOK )
		{
		incapable_log(group_pass, _("Printer is not permitted to print grayscale documents."));
		incapable = TRUE;
		incapable_prohibitions++;
		}

	/*-----------------------------------------------------------------
	** Check to see if the communications channel can pass the set of
	** codes which the job says it contains.
	-----------------------------------------------------------------*/
	{
	int required, available;
	#ifdef GNUC_HAPPY
	required = available = 0;
	#endif

	switch(job.attr.docdata)
		{
		case CODES_UNKNOWN:
			required = 0;
			break;
		case CODES_Clean7Bit:
			required = 1;
			break;
		case CODES_Clean8Bit:
			required = 2;
			break;
		case CODES_Binary:
			required = 3;
			break;
		case CODES_TBCP:
		case CODES_DEFAULT:
			fatal(EXIT_PRNERR_NORETRY, "%s(): %d: assertion failed", function, __LINE__);
			break;
		}

	switch(printer.Codes)
		{
		case CODES_UNKNOWN:
			available = 100;
			break;
		case CODES_Clean7Bit:
			available = 1;
			break;
		case CODES_Clean8Bit:
			available = 2;
			break;
		case CODES_Binary:
		case CODES_TBCP:
			available = 3;
			break;
		case CODES_DEFAULT:
			fatal(EXIT_PRNERR_NORETRY, "%s(): %d: assertion failed", function, __LINE__);
			break;
		}

	if(required > available)
		{
		incapable_log(group_pass, _("Communications channel cannot pass required codes."));
		incapable = TRUE;
		incapable_codes++;
		}
	}

	/*---------------------------------------------------------------------------
	** If the number of pages in this job is known, check to see if this job is
	** outside the allowable range for number of pages for this printer.
	---------------------------------------------------------------------------*/
	if(job.attr.pages > 0 && (job.attr.pages < printer.limit_pages.lower || (printer.limit_pages.upper > 0 && job.attr.pages > printer.limit_pages.upper)))
		{
		incapable_log(group_pass, _("Job has %d pages but allowed range for this printer is %d--%d."), job.attr.pages, printer.limit_pages.lower, printer.limit_pages.upper);
		incapable = TRUE;
		incapable_pageslimit++;
		}

	/*------------------------------------------------------------
	** Check to see if this job is outside the allowable range
	** for number of kilobytes for this printer.
	------------------------------------------------------------*/
	{
	int kilobytes = job.attr.postscript_bytes / 1024;
	if(kilobytes < printer.limit_kilobytes.lower || (printer.limit_kilobytes.upper > 0 && kilobytes > printer.limit_kilobytes.upper))
		{
		incapable_log(group_pass, _("Job is %d kilobytes long but allowed range for this printer is %d--%d."), kilobytes, printer.limit_kilobytes.lower, printer.limit_kilobytes.upper);
		incapable = TRUE;
		incapable_kilobyteslimit++;
		}
	}

	end_of_incapable_log();

	/*------------------------------------------------------------
	** If the printer was judged to be incapable and this is not
	** a group job on the second pass, add a "Reason: " line
	** to the job's queue file so that the user will know why it
	** was arrested.
	------------------------------------------------------------*/
	if(incapable && group_pass < 2)
		{
		char temp[256];			/* string to build reason in */
		int x=0;

		temp[0] = '\0';			/* in case nothing matches */

		if(incapable_fonts)
			{
			if(incapable_fonts > 1)
				snprintf(temp, sizeof(temp), "%d missing fonts", incapable_fonts);
			else
				snprintf(temp, sizeof(temp), "1 missing font");
			x+=strlen(temp);			/* more pointer for later append */
			}

		if(incapable_resources)
			{
			const char *other;

			if(x)						/* if text placed above, */
				temp[x++] = ',';		/* add a comma but no space */

			other = incapable_fonts ? "other " : "";

			if(incapable_resources > 1) /* be grammatical */
				snprintf(&temp[x], sizeof(temp) - x, "%d %smissing rsrcs", incapable_resources, other);
			else
				snprintf(&temp[x], sizeof(temp) - x, "1 %smissing rsrc", other);
			x += strlen(&temp[x]);		/* move pointer for later append */
			}

		if(incapable_requirements)
			{
			if(x)
				temp[x++] = ',';

			if(incapable_requirements > 1)
				snprintf(&temp[x], sizeof(temp) - x, "%d ptr incompatibilities",incapable_requirements);
			else
				snprintf(&temp[x], sizeof(temp) - x, "1 ptr incompatiblity");

			x += strlen(&temp[x]);
			}

		if(incapable_prohibitions)
			{
			if(x)
				temp[x++] = ',';

			if(incapable_prohibitions > 1)
				snprintf(&temp[x], sizeof(temp) - x, "%d ptr prohibitions", incapable_requirements);
			else
				snprintf(&temp[x], sizeof(temp) - x, "1 ptr prohibition");

			x += strlen(&temp[x]);
			}

		if(incapable_codes)
			{
			if(x)
				temp[x++] = ',';

			snprintf(&temp[x], sizeof(temp) - x, "unpassable codes");

			x += strlen(&temp[x]);
			}

		if(incapable_pageslimit)
			{
			if(x)
				temp[x++] = ',';
			snprintf(&temp[x], sizeof(temp) - x, "outside pages limits");
			x += strlen(&temp[x]);
			}

		if(incapable_kilobyteslimit)
			{
			if(x)
				temp[x++] = ',';
			snprintf(&temp[x], sizeof(temp) - x, "outside kilobytes limits");
			x += strlen(&temp[x]);
			}

		if(temp[0])
			give_reason(temp);
		} /* end of if(incapable) */

	if(qline) gu_free(qline);

	DODEBUG_RESOURCES(("%s(): incapable=%s", function, incapable ? "TRUE" : "FALSE"));

	return incapable;
	} /* end of check_if_capable() */

/* end of file */
