/*
** mouse:~ppr/src/ppr/ppr_rcache.c
** Copyright 1995--1999, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Last modified 9 August 1999.
*/

/*
** Resource Cache routines.
**
** These routines are called when "%%BeginResource:" and "%%EndResource"
** comments are seen.  The routines may decide to put the resource
** in the cache.  If the strip resources switch (-S) was used, the
** resource will be removed from the incoming file and will later be
** re-inserted from the cache.
*/

#include "before_system.h"
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"

#include "global_structs.h"
#include "ppr.h"
#include "ppr_infile.h"
#include "ppr_exits.h"

/* Variables shared by begin_resource() and end_resource(): */
static int is_font;			/* fonts receive special treatment in end_resource() */
static int merging;			/* merging type 1 and type 42 */
static char tcache_name[MAX_PPR_PATH];	/* temporary file name */
static char cache_name[MAX_PPR_PATH];	/* name of final cache file */

/* This keeps track of how many resources we have stripped out. */
static int cache_strip_count = 0;
int get_cache_strip_count(void) { return cache_strip_count; }

/* Options set from command line: */
extern gu_boolean option_strip_cache;
extern gu_boolean option_strip_fontindex;
extern enum CACHE_STORE option_cache_store;

/* See ppr_simplify.c */
extern int rgrab;

/*
** This the the resource search list for this operation.  Since we are
** only interested in whether or not this resource is in the cache,
** that is all we check.
*/
static const enum RES_SEARCH search_list[] = { RES_SEARCH_CACHE, RES_SEARCH_FONTINDEX, RES_SEARCH_END };

static gu_boolean cache_exists(const char type[])
    {
    char dirname[MAX_PPR_PATH];
    struct stat statbuf;
    ppr_fnamef(dirname, "%s/%s", CACHEDIR, type);
    return (stat(dirname, &statbuf) >= 0 && statbuf.st_mode & S_IFDIR);
    }

/*
** This routine is called by getline_simplify() in ppr_simplify.c in order
** to allow the resource cache to examine the start of a new resource.
** It is very important that nest_level()==1 when this routine is called;
** it must not be called at the start of a nested resource.
*/
void begin_resource(void)
    {
    const char function[] = "begin_resource";
    char *type = tokens[1];	/* DSC comment type of resource */
    char *name = tokens[2];	/* DSC comment name of resource */
    double version;		/* DSC comment version number of resource */
    int revision;		/* DSC comment revision number of resource */
    gu_boolean is_procset;
    gu_boolean is_good_font;
    int font_features;
    const char *found;
    enum RES_SEARCH where_found;

    if(nest_level() != 1)
	fatal(PPREXIT_OTHERERR, "%s(): nest_level() != 1\n", function);

    /*
    ** If the type or resource or the resource name is missing,
    ** Issue a warning message and don't cache it.
    */
    if(!type || !name)
	{
	warning(WARNING_SEVERE, _("Invalid \"%%%%BeginResource:\" line:\n%s"), line);
	return;
	}

    /* Set flags indicating which type of resource it is for later use. */
    is_procset = FALSE;
    is_font = FALSE;
    if(strcmp(type, "procset") == 0)
    	is_procset = TRUE;
    else if(strcmp(type, "font") == 0)
    	is_font = TRUE;

    /*
    ** If this is a procedure set, read the
    ** version and revision numbers.
    */
    version = 0.0;
    revision = 0;
    if(is_procset && tokens[3] && tokens[4])
	{
	version = gu_getdouble(tokens[3]);
	revision = atoi(tokens[4]);
	}

    #ifdef DEBUG_RESOURCES
    printf("%s(): %s %s %s %d\n", function, type, quote(name), gu_dtostr(version), revision);
    #endif

    /*
    ** If this is a font, eat up line feeds and carriage returns
    ** and then look to make sure it begins with "%!".
    */
    is_good_font = FALSE;
    if(is_font)
    	{
	int c1, c2;

	/* eat whitespace and leave first meaningful character in c1. */
	while((c1 = in_getc()) == '\n' || c1 == '\r') ;

	c2 = in_getc();		/* get second character */
	in_ungetc(c2);		/* put it back */

	in_ungetc(c1);		/* put back the character which was not blank */

	if(c1 == '%' && c2 == '!')
	    is_good_font = TRUE;
	else
	    warning(WARNING_PEEVE, "Dubious font \"%s\" downloaded in print job", name);
    	}

    /*
    ** Search the cache and the font index for this resource.  Notice that if we have
    ** a partial copy of a Macintosh style TrueType font in the cache it takes precedence
    ** over a font in the font index.
    */
    found = noalloc_find_cached_resource(type, name, version, revision, search_list, (int*)NULL, &font_features, &where_found);

    /*
    ** If it was not found or it is a partial Macintosh TrueType font, or
    ** it was found but not in the cache and --cache_store=all is in effect,
    */
    if(!found 	|| (is_font && (merging = truetype_more_needed(font_features)))
		|| (option_cache_store == CACHE_STORE_UNCACHED && where_found != RES_SEARCH_CACHE)
		)
        {
        #ifdef DEBUG_RESOURCES
        printf("%s(): resource is not in cache or is incomplete\n", function);
        #endif

	/* Notice that completing a TrueType font takes priority over "--cache-store=none". */
	if( merging || (option_cache_store != CACHE_STORE_NONE && cache_exists(type)) )
            {
	    #ifdef DEBUG_RESOURCES
	    printf("%s(): resource cache exists and --cache-store is not none\n", function);
	    #endif

            if((!is_procset || version || revision) && (!is_font || is_good_font))
                {
                #ifdef DEBUG_RESOURCES
                printf("%s(): resource is of sufficient quality, will cache\n", function);
                #endif

		/* Build the file name we will eventually give the resource. */
		if(is_procset)
		    ppr_fnamef(cache_name, "%s/procset/%s-%s-%d", CACHEDIR, name, gu_dtostr(version), revision);
		else
		    ppr_fnamef(cache_name, "%s/%s/%s", CACHEDIR, type, name);

		/* Build the name of a file to put it in temporarily. */
	    	ppr_fnamef(tcache_name, "%s/%s/.temp%ld", CACHEDIR, type, (long)getpid());

		/* Open the temporary file to hold the resource. */
		if(!(cache_file = fopen(tcache_name, "w")))
		    fatal(PPREXIT_OTHERERR, _("%s(): Can't open \"%s\", errno=%d (%s)"), function, tcache_name, errno, gu_strerror(errno));

		/*
		** Set rgrab to 1 so that getline_simplify_cache() will know to
		** begin writing the input to the cache file after
		** this line (which is a "%%BeginResource:" line).
		*/
		rgrab = 1;

		/*
		** Since we are putting the resource in the cache, pretend it
		** was found there so that the stripping code below can strip
		** it out if hte --strip-cache option is TRUE.
		*/
		found = "true";
		where_found = RES_SEARCH_CACHE;
	    	} /* if of sufficient quality */
	    } /* if partial truetype, or cache exists and --cache-store not "none" */
	} /* if not found anywhere, or partial truetype, or --cache-store=uncached */

    /*
    ** We are already in an if() which is true if we are going to
    ** copy this resource to the cache or it is already in the
    ** cache.  Now, if we are stripping cached and cachable resources:
    */
    if( found && ((option_strip_cache && where_found == RES_SEARCH_CACHE)
    		|| (option_strip_fontindex && where_found == RES_SEARCH_FONTINDEX)) )
        {
        #ifdef DEBUG_RESOURCES
        printf("%s(): striping this resource out\n", function);
        #endif

        if(where_found == RES_SEARCH_CACHE)
            cache_strip_count++;

        resource(REREF_REMOVED, tokens[1], 2);  /* Mark as removed. */

        if(is_procset)                          /* procsets get version/rev */
            fprintf(text, "%%%%IncludeResource: %s %s %s %d\n",type,quote(name), gu_dtostr(version), revision);
        else                                    /* other types don't */
            fprintf(text, "%%%%IncludeResource: %s %s\n",type,quote(name));

        /*
        ** If getline_simplify_cache() is going to be quietly
        ** copying the input lines into a cache file,
        ** then tell it that it may start immediately
        ** rather than on the line after this one.
        */
        if(rgrab)
            rgrab = 2;

        /*
        ** Recursively call getline_simplify_cache() until
        ** the end of the resource or the end of the file.
        */
        while(nest_level() && !in_eof())
            getline_simplify_cache();

        getline_simplify_cache();        /* and leave a fresh line in line[] */
        }

    #ifdef DEBUG_RESOURCES
    printf("%s() done, rgrab = %d\n", function, rgrab);
    #endif
    } /* end of begin_resource() */

/*
** This routine is called by getline_simplify() in ppr_old2new.c
** in order to tell the cache that the end of a resource has been
** encountered.
**
** It is very important that nest_level()==1 when this routine is called;
** it must not be called at the end of a nested resource.
*/
void end_resource(void)
    {
    const char function[] = "end_resource";

    #ifdef DEBUG_RESOURCES
    printf("%s(): rgrab=%d, is_font=%d, merging=%d\n", function, rgrab, is_font, merging);
    #endif

    /*
    ** If we are not copying any part of this file into the cache,
    ** we don't have anything to do here.
    */
    if(rgrab == 0)
	return;

    /* Sanity check. */
    if(nest_level() != 1)
	fatal(PPREXIT_OTHERERR, "%s(): nest_level() != 1\n", function);

    /* Close the file into which we were collecting the resource. */
    fclose(cache_file);

    /* Set the flag so no attempt will be made to add more lines. */
    rgrab = 0;				/* 0, not "TRUE"! */

    /* If this is a font, */
    if(is_font)
    	{
	char *fontname;

	/* set fontname to point to the file name part of old */
	if( (fontname = strrchr(cache_name, '/')) == (char*)NULL )
	    fatal(PPREXIT_OTHERERR, "%s(): invalid cache_name", function);

	/* Have we decided to try to merge Type1 and Type42 versions of the font? */
	if(merging)
	    {
	    truetype_merge_fonts(fontname, cache_name, tcache_name);
    	    }

	/* If not, just set the flags to indicate whether this is a Mac TT font. */
    	else
	    {
	    if( truetype_set_fontmode(tcache_name) != -1 )	/* If valid font, */
		{
		rename(tcache_name, cache_name);		/* move into place */
		}
	    else						/* otherwise, */
		{
	    	unlink(tcache_name);				/* discard it. */
		warning(WARNING_SEVERE, _("Invalid Macintosh TrueType PostScript font discarded"));
		}
	    }
    	}

    /*
    ** Not a font, must be some other kind or resource, just
    ** move it into place.
    */
    else
    	{
	rename(tcache_name, cache_name);	/* move to final name */
	}

    } /* end of end_resource() */

/*
** This routine is called if nest_level() is non-zero at the end of the job.
** It gets rid of any resource cache file we were trying to create.
** This is done because a resource which does not end before the end
** of the file is corrupt.
**
** This is also called by file_cleanup().
*/
void abort_resource(void)
    {
    if(rgrab)
	{
	fclose(cache_file);
	unlink(tcache_name);
	}
    } /* end of abort_resource() */

/* end of file */

