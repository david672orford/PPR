/*
** mouse:~ppr/src/pprdrv/pprdrv_res.c
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
** Last modified 19 July 2001.
*/

/*
** Code for including and striping out resources.
*/

#include "before_system.h"
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "global_structs.h"
#include "interface.h"
#include "pprdrv.h"

static const enum RES_SEARCH add_resource_search_list[] =
	{
	RES_SEARCH_FONTINDEX,
	RES_SEARCH_CACHE,
	RES_SEARCH_END
	};

/*
** include_resource() is called whenever an "%%IncludResource:" comment is
** encountered.	 If the resource in question is in the printer, do
** not include it, just send the comment to the printer; if the
** resource is in our cache, insert it, bracketed by "%%Begin(End)Resource:"
** comments; if it is not in the printer and not in the cache, then
** just send the comment to the printer in the faint hope that another
** intervening spooler will be able to supply it.
**
** include_resource() is a wrapper function for internal_include_resource().
*/
static void include_resource_2(const char type[], const char name[], double version, int revision, int forcing_prolog, int forcing_docsetup)
	{
	FILE *ifile;						/* Input file (resource cache file). */
	char tline[MAX_LINE+1];				/* Temporary line for reading resource cache files. */
	int tline_len;
	struct DRVRES *d;					/* pointer to resource list entry */
	int x;								/* index into resource list */

	DODEBUG_RESOURCES(("include_resource_2(type=\"%s\", name=\"%s\", version=%s, revision=%d)", type, name, gu_dtostr(version), revision));

	/*
	** Search the resource list to find
	** the resource in question.
	*/
	for(x=0; x < drvres_count; x++)
		{
		d = &drvres[x];					/* make a handy pointer */

		DODEBUG_RESOURCES(("trying %s %s %s %d", d->type, d->name, gu_dtostr(d->version), revision));

		if(strcmp(d->type,type))		/* We aren't interested in things */
			continue;					/* which are of the wrong type. */

		/*
		** If the current name is what we are looking for or the
		** former name is what we are looking for,
		*/
		if( (strcmp(d->name,name)==0 || (d->former_name!=(char*)NULL && strcmp(d->former_name,name)==0))
				&& d->version==version && d->revision==revision )
			{
			DODEBUG_RESOURCES(("Match found in drvres table!"));

			/*
			** If this resource was forced into the prolog section
			** and we are not carrying out that order right now,
			** delete it from this position.
			*/
			if(!forcing_prolog && d->force_into_prolog)
				{
				DODEBUG_RESOURCES(("Deleting resource here because forced into prolog"));
				return;
				}

			/*
			** If this resource was forced into the docsetup section
			** and this is not the docsetup section, ignore this
			** request to include it since doing so would
			** be redundant.
			*/
			if(!forcing_docsetup && d->force_into_docsetup)
				{
				DODEBUG_RESOURCES(("Deleting resource here because forced into docsetup"));
				return;
				}

			/*
			** If resource has a former name, use the new name when loading it
			** instead of the name from the origional DSC comment.
			*/
			if(d->former_name)					
				name = d->name;					

			/*
			** If a cache file was identified, insert its contents.
			*/
			if(d->filename)
				{
				if(d->dot_ttf)
					{
					printer_printf("%%%%BeginResource: font %s\n", quote(name));
					send_font_tt(d->filename);
					printer_putline("%%EndResource");
					}
				else if(d->mactt)
					{
					printer_printf("%%%%BeginResource: font %s\n",quote(name));
					send_font_mactt(d->filename);
					printer_putline("%%EndResource");
					}
				else
					{
					int c;

					if(strcmp(type, "procset") == 0)
						printer_printf("%%%%BeginResource: %s %s %s %d\n",type,quote(name),gu_dtostr(version),revision);
					else
						printer_printf("%%%%BeginResource: %s %s\n",type,quote(name));

					/* Open the cache file for read */
					if(!(ifile = fopen(d->filename, "r")))
						fatal(EXIT_JOBERR, "The resource file \"%s\" can't be opened", d->filename);

					/*
					** If the first character is 128, this is a PFB
					** font which we must expand.
					*/
					if((c = fgetc(ifile)) != EOF)
						{
						ungetc(c,ifile);
						if(c == 128)
							{
							send_font_pfb(d->filename,ifile);
							}
						else
							{
							while((tline_len = fread(tline, sizeof(char), MAX_LINE+1, ifile)))
								 printer_write(tline,tline_len);		/* Copy it from the cache file. */
							}
						}

					/* close the cache file */
					fclose(ifile);

					printer_putline("%%EndResource");	/* close commented section */
					} /* end of if not truetype */
				} /* end of if in cache */

			/*
			** Otherwise, resource is in printer, no code needed, just a comment.
			*/
			else
				{
				if(strcmp(type, "procset") == 0)
					printer_printf("%%%%IncludeResource: %s %s %s %d\n",type,quote(name),gu_dtostr(version),revision);
				else
					printer_printf("%%%%IncludeResource: %s %s\n",type,quote(name));
				}

			/*
			** If substitute (fonts only), emmit PostScript to define the font
			** under the substituted font's name.
			*/
			if(d->former_name)			
				{
				printer_printf("/%s /%s findfont %%PPR\n", d->former_name, d->name);

				if(d->subst_code)						/* If special substitution code, insert it. */
					printer_printf("%s makefont %%PPR user supplied matrix\n", d->subst_code);

				printer_putline("dup maxlength 1 add dict /ppr_subfont exch def %PPR");
				printer_putline("{exch dup /FID ne %PPR");
				printer_putline(" {exch ppr_subfont 3 1 roll put} %PPR");
				printer_putline(" {pop pop} %PPR");
				printer_putline(" ifelse %PPR");
				printer_putline("} forall %PPR");
				printer_putline("ppr_subfont %PPR");
				printer_putline("definefont pop %PPR");
				}

			return;
			} /* end of if this is the correct resource */
		} /* end of loop to search resource list */

	/*
	** This should eventualy be commented out because the process of job
	** splitting may leave jobs with "%%IncludeResource: comments in their
	** prolog or document setup sections for resources which one or more
	** fragments do not require.
	*/
	fatal(EXIT_JOBERR, "include_resource_2(): resource \"%s %s\" is not in the resource list", type, name);
	} /* end of include_resource_2() */

/*
** This wrapper routine is called when an "%%IncludeResource:"
** comment is encountered in the file.
*/
void include_resource(void)
	{
	const char *type = tokens[1];		/* resource type, font, procset, etc. */
	const char *name = tokens[2];		/* resource name, Courier, Times-Roman, etc. */
	double version;
	int revision;

	if(!type || !name)
		return;

	/* If resource is a procset with version and revision numbers, */
	if(strcmp(type, "procset") == 0 && tokens[3] && tokens[4])
		{
		version = gu_getdouble(tokens[3]);
		sscanf(tokens[4], "%d", &revision);
		}
	/* If not a procset or missing, use dummy values. */
	else
		{
		version = 0.0;
		revision = 0;
		}

	include_resource_2(type, name, version, revision, FALSE, FALSE);
	} /* end of include_resource() */

/*
** This routine is called when a "%%BeginResource:" comment is encountered.
** It copies the whole resource from "%%BeginResource:" to "%%EndResource".
** It also may choose to strip out the resource if it is already in the printer.
**
*/
void begin_resource(FILE *infile)
	{
	const char function[] = "begin_resource";
	const char *type = tokens[1];
	const char *name = tokens[2];
	struct DRVRES *d = NULL;
	int x;

	/* If the "%%BeginResource:" line had the necessary parameters, find the
	   drvres[] entry so we know if we are moving the resource.
	   (For example, fonts are sometimes moved into the document setup
	   section.) */
	if(type && name)
		{
		for(x=0; x < drvres_count; x++)
			{
			d = &drvres[x];
			if(strcmp(drvres[x].type, type) == 0 && strcmp(drvres[x].name, name) == 0)
				{
				d = &drvres[x];
				break;
				}
			}
		}

	/* Only PPR bugs or the absence of a parameter can cause the above search to fail. */
	if(!d && type && name)
		fatal(EXIT_JOBERR, "%s(): assertion failed", function);

	/* If should be stripped from this location, */
	if(d && (
		(d->force_into_prolog && !doing_prolog)
		|| (d->force_into_docsetup && !doing_docsetup)
		|| (job.StripPrinter && strcmp(type, "font") == 0 && ppd_font_present(name))
		)
		)
		{
		/* Replace with comment. */
		printer_printf("%%%%IncludeResource: %s %s\n", type, name);
		/* Throw away content. */
		do { } while(strcmp(line, "%%EndResource") && dgetline(infile));
		}

	/* If should not be stripped, */
	else
		{
		/* Copy the content, starting with the "%%BeginResource:" line
		   which is still in line[].  Note that the calls to dgetline()
		   could result in recursive calls to this function.  That
		   should not be a problem. */
		do	{
			printer_write(line, line_len);
			if(! line_overflow)
				printer_putc('\n');
			} while(strcmp(line, "%%EndResource") && dgetline(infile));
		}

	} /* end of begin_resource() */

/*
** Insert additional resources in the prolog.  These additional
** resources may be required due to N-Up or the inclusion of
** type 42 fonts.
**
** It is also possible that the resource was already included
** in an individual page description and this routine will move
** it into the prolog.
**
** This will also insert non-font resources for which fixinclude
** is set.	An example would be a procset which was declared needed
** but for which no "%%IncludeResource: procset" line appeared.
*/
void insert_extra_prolog_resources(void)
	{
	int x;
	struct DRVRES *d;

	for(x=0; x < drvres_count; x++)
		{
		d = &drvres[x];
		if(d->force_into_prolog)
			{
			include_resource_2(d->type, d->name, d->version, d->revision, TRUE, FALSE);
			}
		}
	} /* end of insert_extra_prolog_resources() */

/*
** Insert those fonts for which no ``%%IncludeResource:''
** comment was origionally provided.
**
** This routine is called during processing of the
** Document Setup Section or at the top of the document in the case
** of non-conforming documents.
**
** This routine will also include a resource if force_into_docsetup is TRUE,
** even if it is not a font.  This is used for resource optimization.
*/
void insert_noinclude_fonts(void)
	{
	int x;
	struct DRVRES *d;

	for(x=0; x < drvres_count; x++)
		{
		d = &drvres[x];
		if(d->force_into_docsetup)
			include_resource_2(d->type, d->name, d->version, d->revision, FALSE, TRUE);
		}
	} /* end of insert_noinclude_fonts() */

/*
** Write the ``%%DocumentNeeded(Supplied)Resources:'' comments.
** Remember, these comments were stript out when the job
** entered the queue.
*/
void write_resource_comments(void)
	{
	int x;
	int started;
	struct DRVRES *d;

	/*
	** Enumerate the resources which we hope somebody else,
	** such as the printer, will supply for us.	 This will
	** normally be a list of fonts.
	*/
	for(started=FALSE,x=0; x < drvres_count; x++)
		{
		d = &drvres[x];
		if(d->needed)					/* use only if ``needed'' */
			{
			if(started)					/* if already started, */
				{						/* use */
				printer_puts("%%+ ");	/* continuation comment */
				}
			else
				{
				started = TRUE;
				printer_puts("%%DocumentNeededResources: ");
				}
			if(strcmp(d->type, "procset") == 0)					/* procsets */
				printer_printf("%s %s %s %d\n",
					d->type, quote(d->name),					/* get */
					gu_dtostr(d->version), d->revision);				/* version info */
			else												/* other */
				printer_printf("%s %s\n", d->type, quote(d->name));	 /* resources */
			}													/* don't */
		}

	/*
	** Enumerate the resources which we are including in the document.
	** This will normally be a list of procedure sets and downloaded
	** fonts.
	*/
	for(started=FALSE,x=0; x < drvres_count; x++)
		{
		d = &drvres[x];
		if(! d->needed)					/* use only if not ``needed'' */
			{
			if(started)					/* if already started, */
				{						/* use */
				printer_puts("%%+ ");	/* continuation comment */
				}
			else
				{
				started = TRUE;
				printer_puts("%%DocumentSuppliedResources: ");
				}
			if(strcmp(d->type,"procset")==0)				/* procsets */
				printer_printf("%s %s %s %d\n",
					d->type,quote(d->name),	   /* get */
					gu_dtostr(d->version),d->revision );	   /* version info */
			else											/* other */
				printer_printf("%s %s\n",d->type,quote(d->name));  /* resources */
			}												/* don't */
		}

	} /* end of write_resource_comments() */

/*============================================================================
** Routines for manipulating the DRVRES structure array.
** This array holds a list of the resources used in the document.
============================================================================*/

/*
** Add a DRVRES record and return a pointer to it.
*/
struct DRVRES *add_drvres(int needed, int fixinclude, const char type[], const char name[],
		double version, int revision)
	{
	struct DRVRES *d;

	/* If we need more space in the array, enlarge it. */
	if(drvres_count == drvres_space)
		{
		drvres_space += 100;
		drvres = (struct DRVRES *)gu_realloc(drvres, drvres_space, sizeof(struct DRVRES));
		}

	/* Take the next drvres[] record position. */
	d = &drvres[drvres_count++];

	d->needed = needed;					/* True if resource not still in PostScript file. */
	d->type = type;						/* resource type string: "font", "procset", etc. */
	d->name = name;						/* resource name string: "Times-Roman", "Trincoll-dmm-nup", etc. */
	d->version = version;				/* floating point version number: 3.1, 0.0, etc. */
	d->revision = revision;				/* integer revision number: 15, 0, etc. */
	d->former_name = (char*)NULL;		/* former name (if a substitute font) */
	d->subst_code = (char*)NULL;		/* PostScript code to insert during font substitution */
	d->filename = (char*)NULL;			/* Name of file to load resource from (none as yet). */
	d->dot_ttf = FALSE;					/* initially, not a TrueType font */
	d->mactt = FALSE;					/* not known to be a Macintosh Type1/Type42 font */
	d->force_into_docsetup = FALSE;		/* don't `optimize' */
	d->force_into_prolog = FALSE;		/* initially, don't add to prolog */

	if(fixinclude)
		{
		if(strcmp(type, "font") == 0)
			d->force_into_docsetup = TRUE;
		else
			d->force_into_prolog = TRUE;
		}

	return d;							/* return pointer to what we made */
	} /* end of add_drvres() */

/*
** Add a resource to the prolog.  If it is already elsewhere
** in the document, force it into the prolog.  This is used
** in the N-Up code to add the resources for N-Up, and
** in want_ttrasterizer() to request the 68000 TrueType rasterizer.
**
** Return 0 if we suceeded, -1 if we couldn't find it.
*/
int add_resource(const char type[], const char name[], double version, int revision)
	{
	struct DRVRES *d;
	char *fnptr;
	int x;
	int features;

	/*
	** See if the resource is already in the document.	If it is, make
	** sure it is loaded in the prolog or docsetup section.
	*/
	for(x=0; x < drvres_count; x++)
		{
		d = &drvres[x];
		if(strcmp(d->type, type) == 0 && strcmp(d->name, name) == 0)
			{
			if(strcmp(type, "font") == 0)
				d->force_into_docsetup = TRUE;
			else
				d->force_into_prolog = TRUE;
			return 0;
			}
		}

	/*
	** If we have it in the cache, then we will be downloading it.
	** Notice that we say it is not a "needed" resource in the sense that
	** it will not be missing from the data that we send to the
	** printer.
	*/
	fnptr = NULL;
	features = 0;
	if((strcmp(type, "font") == 0 && ppd_font_present(name))
				|| (fnptr = find_cached_resource(type, name, version, revision, add_resource_search_list, (int*)NULL, &features, NULL))
				)
		{
		d = add_drvres(FALSE, FALSE, type, name, version, revision);
		d->filename = fnptr;

		if(strcmp(type, "font") == 0)
			d->force_into_docsetup = TRUE;
		else
			d->force_into_prolog = TRUE;

		/* code is needed here for TrueType */

		return 0;
		}

	return -1;
	} /* end of add_resource() */

/* end of file */

