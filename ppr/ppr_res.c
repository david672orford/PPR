/*
** mouse:~ppr/src/ppr/ppr_res.c
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
** Last modified 10 September 2001.
*/

/*
** Resource comment interpretation routines.
**
** This module is involved in processing "%%DocumentRequiredResources:",
** "%%DocumentSuppliedResources:", and other lines of that sort.
*/

#include "before_system.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "global_structs.h"
#include "ppr.h"

/*
** Internal routine to return a resource name as ASCII text.
**
** Procedure set names are returned in the form:
**
** procset _name_ _version_ _revision_
**
** Other resource names are returned in the form:
**
** _type_ _name_
*/
static char *resname_to_str(const char *type, const char *name, double version, int revision)
    {
    static char *result = (char*)NULL;
    static int result_len = 0;

    const char *version_str = (char*)NULL;
    char revision_str[11];
    int len;

    len = strlen(type) + strlen(name) + 3;	/* two spaces and a NULL */

    if(strcmp(type, "procset") == 0)
    	{
    	version_str = gu_dtostr(version);
	snprintf(revision_str, sizeof(revision_str), "%d", revision);

    	len += strlen(version_str);
    	len += strlen(revision_str);
    	len += 2;			/* plus two spaces */
    	}

    if(result_len < len)		/* If the existing result space is */
    	{				/* not big enough, get a new one. */
    	if(result)			/* Free the old one if it exists. */
    	    gu_free(result);
    	result = (char*)gu_alloc(len,sizeof(char));
	result_len = len;		/* Update length of result space. */
    	}

    if(version_str)			/* If Procset, */
    	snprintf(result, result_len, "%s %s %s %s", type, name, version_str, revision_str);
    else				/* other kind if resource */
    	snprintf(result, result_len, "%s %s", type, name);

    return result;
    } /* end of resname_to_str() */

/*
** Called on each reference to a resource,
** whether needed, provided, or included.
** This is not called on %%BeginResource:
** Return the number of "words" eaten.
**
** This routine does not mind if it is fed NULL arguments.
*/
int resource(int reftype, const char *restype, int first)
    {
    struct Resource *resource;  /* pointer to structure we work on */
    char *resname;              /* resource name */
    double version;             /* version number */
    int revision;               /* backward compatible revision number */
    int x;
    int rval;                   /* number of words `eaten' */
    char *ptr;

    #ifdef DEBUG_RESOURCES_DETAILED
    printf("resource(): %d %s %s %s %s\n",
    	reftype,
    	restype,
    	tokens[first]?tokens[first]:"<NULL>",
	tokens[first+1]?tokens[first+1]:"<NULL>",
	tokens[first+2]?tokens[first+2]:"<NULL>" );
    #endif

    resname = tokens[first];	/* resource name is first */

    if(restype == (char*)NULL || resname == (char*)NULL)
	return 1;               /* just a guess value to cludge things */

    /*
    ** Procedure sets are supposed to have version numbers,
    ** if they do not, it is an error.
    ** RBIIpp. 637,638 indicates that the revision number is optional.
    ** If it is not present we treat it as zero.  It seems that doing
    ** so will fulfill all of the requirements.
    */
    if(strcmp(restype, "procset") == 0)
      {
      if( (ptr=tokens[first+1])!=(char*)NULL			/* 1 param present */
	    && ptr[strspn(ptr, "0123456789.")] == (char)NULL )	/* all digits & dec pt */
	{
	version = gu_getdouble(tokens[first+1]);

	if( (ptr=tokens[first+2]) != (char*)NULL			/* another param present */
		&& ptr[strspn(ptr, "0123456789")] == (char)NULL)	/* all digits */
	    {
	    revision = atoi(tokens[first+2]);
	    rval = 3;		/* return 3 to eat 3 parameters */
	    }
	else
	    {
	    revision = 0;
	    rval = 2;
	    }
	}
    else			/* If version number not present, */
	{
	warning(WARNING_SEVERE, _("Procset \"%s\" has no version and revision numbers"), resname);
	version = 0;		/* user dummy values for both */
	revision = 0;
	rval = 1;		/* return 1 to eat one parameter */
	}
      }
    else                        /* If not a procedure set, */
      {                         /* use dummy values */
      version = 0;
      revision = 0;
      rval = 1;			/* and only eat 1 word. */
      }

    #ifdef DEBUG_RESOURCES
    printf("resource(): ref=%d type=%s name=%s version=%s revision=%d\n",
	    reftype,restype,quote(resname),gu_dtostr(version),revision);
    #endif

    /* search for the resource */
    for(x=0; x < thing_count; x++)
	{
	if(things[x].th_type == TH_RESOURCE)
	    {
	    resource = (struct Resource*)things[x].th_ptr;
	    if( ( strcmp(resource->R_Type, restype) == 0 ) &&
			( strcmp(resource->R_Name, resname) == 0 ) &&
			( resource->R_Version == version ) &&
			( resource->R_Revision == revision ) )
		break;
	    }
	}

    /* if wasn't found, add it */
    if(x == thing_count)
	{
	#ifdef DEBUG_RESOURCES_DETAILED
	printf("resource(): this is 1st reference\n");
	#endif

	things_space_check();			/* make space in the array */

	thing_count++;
	things[x].th_type = TH_RESOURCE;	/* this thing is resource */
	resource = (struct Resource*)gu_alloc(1, sizeof(struct Resource));
	things[x].th_ptr = (void*)resource;	/* point thing ptr to it */
	things[x].R_Flags = 0;			/* initially, clear flags */
	resource->R_Type = gu_strdup(restype);
	resource->R_Name = gu_strdup(resname);
	resource->R_Version = version;
	resource->R_Revision = revision;
	}

    /* note our reference to it */
    things[x].R_Flags |= reftype;               /* or our reference into it */
    if(reftype & REREF_PAGE)                    /* add to the bitmap */
	set_thing_bit(x);                       /* for this page */

    /* return with the value computed above */
    return rval;
    } /* end of resource() */

/*
** This is called whenever a new resource comment is found in the
** trailer section which supersedes a previous one.  It clears
** the indicated flag on all resources of the indicated type.
** If the type is specified as a NULL pointer, it clears the
** indicated flag on all resources.
*/
void resource_clear(int reftype, const char *restype)
    {
    int x;
    struct Resource *resource;

    #ifdef DEBUG_RESOURCES
    printf("resource_clear(reftype = %d, restype = \"%s\")\n", reftype, restype != (char*)NULL ? restype : "<NULL>");
    #endif

    for(x=0; x < thing_count; x++)
	{
	if(things[x].th_type == TH_RESOURCE)
	    {
	    resource = (struct Resource*)things[x].th_ptr;

	    if(restype == (char*)NULL || strcmp(restype, resource->R_Type) == 0)
	    	{
		#ifdef DEBUG_RESOURCES_DETAILED
		printf("resource_clear(): clearing %s %s\n", resource->R_Type, resource->R_Name);
		#endif
		things[x].R_Flags &= ~reftype;
	    	}
	    }
	}
    } /* end of resource_clear() */

/*
** Write the page resources to -pages and clear for the next page
*/
void dump_page_resources(void)
    {
    int started = FALSE;
    int x;
    struct Resource *resource;

    #ifdef DEBUG_RESOURCES_DETAILED
    printf("dumping page resources\n");
    #endif

    for(x=0;x<thing_count;x++)			/* step thru the recorded `things' */
	{
	if(things[x].th_type != TH_RESOURCE)	/* pay attention only to resources */
	    continue;

	resource=(struct Resource *)things[x].th_ptr;

	if(things[x].R_Flags & REREF_PAGE)	/* If the comments explicitly stated that */
	    {				/* the resource in question is used in this page, */
	    if(!started)			/* then emmit a new comment to replace the one */
		{				/* we destroyed. */
		fputs("%%PageResources: ",page_comments);
		#ifdef DEBUG_RESOURCES_DETAILED
		printf("%%%%PageResources: ");
		#endif
		started=TRUE;
		}
	    else				/* Resources after the first one */
		{				/* appear on continuation lines. */
		fputs("%%+ ",page_comments);
		#ifdef DEBUG_RESOURCES_DETAILED
		printf("%%%%+ ");
		#endif
		}

	    /*
	    ** If this is a procset, print with version number,
	    ** otherwise, print without.
	    */
	    if(strcmp(resource->R_Type,"procset")==0)
		{
		fprintf(page_comments, "%s %s %s %d\n",
			resource->R_Type, quote(resource->R_Name),
			gu_dtostr(resource->R_Version),resource->R_Revision );
		#ifdef DEBUG_RESOURCES_DETAILED
		printf("%s %s %s %d\n",
			resource->R_Type, quote(resource->R_Name),
			gu_dtostr(resource->R_Version),resource->R_Revision );
		#endif
		}
	    /*
	    ** If this is not a procedure set, print
	    ** its type and name but no version number.
	    */
	    else
		{
		fprintf(page_comments, "%s %s\n",
			resource->R_Type, quote(resource->R_Name) );
		#ifdef DEBUG_RESOURCES_DETAILED
		printf("%s %s\n", resource->R_Type, quote(resource->R_Name));
		#endif
		}

	    things[x].R_Flags&=(~REREF_PAGE);   /* clear the ref flag */
	    } /* end of if page reference */
	} /* end of for(;;) */

    #ifdef DEBUG_RESOURCES_DETAILED
    printf("done\n");
    #endif
    } /* end of dump_page_resources() */

/*
** This routine is called after the entire document has been read.  It
** examines those elements of the things[] array which represent
** resources, looking for inconsistencies.
**
** If the resource references are inconsistent, emmit warnings and try
** to take a guess.  If comments like "%%DocumentFonts:" appear, try
** to determine if they mean the resources are included or required.
*/
void rationalize_resources(void)
    {
    int x;
    struct Thing *t;
    struct Resource *r;

    for(x=0; x<thing_count; x++)
	{
	t = &things[x];				/* Make a pointer for easy reference to the thing. */

	if(t->th_type != TH_RESOURCE)		/* We are only interested */
	    continue;                           /* in resources, not media, requirements, etc. */

	if(t->R_Flags == 0)			/* short circuit deleted resources */
	    continue;

	r=(struct Resource *)things[x].th_ptr;	/* Cast the thing object pointer to a resource pointer. */

	/*
	** To supply a resource at one point in the document
	** and to ask to have it inserted at another point
	** is truly bizzar behavior.
	*/
	if( (t->R_Flags & REREF_INCLUDE)
		    && (t->R_Flags & REREF_REALLY_SUPPLIED) )
	    {
	    warning(WARNING_SEVERE,
		_("Resource \"%s\" has both %%%%Include and %%%%Begin"),
		resname_to_str(r->R_Type,r->R_Name,r->R_Version,r->R_Revision));
	    }

	/*
	** To say that a resource is both present in the print job
	** and not present indicates serious confusion on the
	** part of the document code generator.
	*/
	if( (t->R_Flags & REREF_NEEDED)
		    && (t->R_Flags & REREF_SUPPLIED) )
	    {
	    warning(WARNING_SEVERE,
		_("Resource \"%s\" declared both Supplied and Needed"),
		resname_to_str(r->R_Type,r->R_Name,r->R_Version,r->R_Revision));
	    }

	/*
	** To say that a resource is supplied and then to leave
	** it out or not to mark it with "%%Begin(End)Resource"
	** indicates serious problems.
	*/
	if( (t->R_Flags & REREF_SUPPLIED)
		    && !(t->R_Flags & REREF_REALLY_SUPPLIED) )
	    {
	    warning(WARNING_SEVERE,
		_("Resource \"%s\" declared Supplied but isn't included"),
		resname_to_str(r->R_Type, r->R_Name, r->R_Version, r->R_Revision));
	    t->R_Flags &= ! REREF_SUPPLIED; /* kinda fix it */
	    }

	/*
	** To say that the printer or spooler must have a resource
	** and then to leave out the comment which tells where to
	** include it is _very_ serious.
	**
	** We will make a best effort to fix it by telling
	** pprdrv to download it in the document setup section.
	*/
	if( (t->R_Flags & REREF_NEEDED) && !(t->R_Flags & REREF_INCLUDE) )
	    {
	    warning(WARNING_SEVERE,
		_("Resource \"%s\" declared Needed but no %%%%Include"),
		resname_to_str(r->R_Type, r->R_Name, r->R_Version, r->R_Revision));
	    t->R_Flags |= REREF_FIXINCLUDE;	/* we will fix this is pprdrv */
	    }

	/*
	** If a resource has a declaration which does not specify
	** if it is "Needed" or "Supplied", try to figure it out.
	** If there is no clue and the resource is a font, guess
	** that the font is "Needed", for other resource types
	** issue a warning.
	**
	** This must come before the tests for undeclared resources.
	*/
	if( (t->R_Flags & REREF_UNCLEAR)
		&& ! ( (t->R_Flags & REREF_NEEDED)
		    || (t->R_Flags & REREF_SUPPLIED) ) )
	    {
	    if( t->R_Flags & REREF_REALLY_SUPPLIED )
		{
		t->R_Flags |= REREF_SUPPLIED;
		}
	    else if(t->R_Flags & REREF_INCLUDE)
		{
		t->R_Flags |= REREF_NEEDED;
		}
	    else
		{
		if(strcmp(r->R_Type, "font") == 0)	/* fonts might be declared */
		    {					/* in */
		    t->R_Flags |= REREF_NEEDED;		/* DSC 1.0 fashion. */
		    t->R_Flags |= REREF_FIXINCLUDE;

		    /* If "%%IncludeFont:" exists in this DSC
		       version, issue a warning. */
		    if(qentry.attr.DSClevel >= 2.0)
		        warning(WARNING_SEVERE, _("No %%%%IncludeFont for font \"%s\""), r->R_Name);
		    }
		else
		    {
		    warning(WARNING_SEVERE,
			_("Resource \"%s\" has no %%%%Begin or %%%%Include"),
				resname_to_str(r->R_Type, r->R_Name, r->R_Version, r->R_Revision));
		    }
		}
	    }

	/*
	** To have a comment indicating where to include a resource
	** but to failed to declair it as a document needed resource
	** is careless, but we can fix it, so it is not a severe warning.
	*/
	if( (t->R_Flags & REREF_INCLUDE)
		    && !(t->R_Flags & REREF_NEEDED) )
	    {
	    warning(WARNING_PEEVE,
		_("Resource \"%s\" has %%%%Include but never declared"),
		resname_to_str(r->R_Type,r->R_Name,r->R_Version,r->R_Revision));

	    t->R_Flags |= REREF_NEEDED;  /* fix it */
	    }

	/*
	** To include a resource and to fail to declare it is careless,
	** but we can fix it, so this is not a severe warning.
	*/
	if( (t->R_Flags & REREF_REALLY_SUPPLIED)
		&& !(t->R_Flags & REREF_SUPPLIED) )
	    {
	    warning(WARNING_PEEVE,
		_("Resource \"%s\" supplied but never declared"),
		resname_to_str(r->R_Type, r->R_Name, r->R_Version, r->R_Revision));

	    t->R_Flags |= REREF_SUPPLIED;   /* fix it */
	    }

	/*
	** If we removed the resource from the file, change it
	** From "Supplied" to "Needed".
	*/
	if( t->R_Flags & REREF_REMOVED )
	    {
	    t->R_Flags &= ( ! REREF_SUPPLIED );
	    t->R_Flags |= REREF_NEEDED;
	    }

	} /* end of for loop */

    } /* end of rationalize_resources() */

/*
** Write one "Res:" line into the queue file for each resource.
**
** The format of a "Res:" line is:
** "Res: ?NEEDED ?ADDINCLUDE TYPE NAME VERSION REVISION"
**
** These lines will be used by pprdrv to determine if a file
** can be printed and to re-construct the DSC comments.
*/
void write_resource_lines(FILE *out, int fragment)
    {
    int x, y, c;
    const struct Resource *resource;

    /* list the needed resources */
    for(x=0; x<thing_count; x++)
	{
	/* Skip things which are not resources. */
	if(things[x].th_type != TH_RESOURCE)
	    continue;

	/* Skip things which are the result of superseded comments. */
	if(things[x].R_Flags == 0)
	    continue;

	/* Ignore resources not in this sub-job. */
	if( ! is_thing_in_current_fragment(x, fragment) )
	    continue;

	resource = (struct Resource *)things[x].th_ptr;

	fprintf(out, "Res: %d %d %s \"",
		things[x].R_Flags & REREF_NEEDED ? 1 : 0,
		things[x].R_Flags & REREF_FIXINCLUDE ? 1 : 0,
		resource->R_Type);
	for(y=0; (c= resource->R_Name[y]); y++)
	    {
	    if(c == '\\' || c == '\"')		/* write this part quoted */
	    	fputc('\\', out);
	    fputc(c, out);
	    }
	fprintf(out, "\" %s %d\n",
		gu_dtostr(resource->R_Version), resource->R_Revision);
	}

    } /* end of write_resource_lines() */

/* end of file */
