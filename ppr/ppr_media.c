/*
** mouse:~ppr/src/ppr/ppr_media.c
** Copyright 1995--2002, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Last modified 8 March 2002.
*/

/*
** Media comment routines.
**
** One of these is called every time a media comment is encountered
** in the input file, others are called when it is necessary to dump
** lists of the required media into the queue file.
*/

#include "before_system.h"
#include <unistd.h>
#include <stdlib.h>
#include <memory.h>		/* NT with Hippix needs this */
#include <errno.h>
#include <string.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "global_structs.h"
#include "ppr.h"
#include "ppr_exits.h"
#include "respond.h"
#include "ppr_gab.h"


extern const char *default_medium;	/* in ppr_main.c */
extern struct Media guess_media;	/* in ppr_main.c */

/* We use this so we don't have to load the math library. */
#define FABS(x) ((x) >= 0 ? (x) : -(x))

/*
** Record a reference to a media type requirment.  This will be
** as a result of a "%%DocumentMedia:" or "%%PageMedia:" comment.
**
** The parameter "reftype" is either MREF_DOC or MREF_PAGE.
*/
void media(int reftype, int first)
    {
    #ifdef DEBUG_MEDIA_MATCHING
    printf("media(reftype = %d, first = %d): %s\n", reftype, first, tokens[first] ? tokens[first] : "NULL");
    #endif

    if(reftype == MREF_DOC)	/* if document wide media reference */
	{
	struct Media *newmedia;	/* pointer to new media structure */

	/*
	** Make sure the comment has all the proper
	** parameters.
	*/
	if(tokens[first] == (char*)NULL
		|| tokens[first+1] == (char*)NULL
		|| tokens[first+2] == (char*)NULL
		|| tokens[first+3] == (char*)NULL
		|| tokens[first+4] == (char*)NULL
		|| tokens[first+5] == (char*)NULL )
	    {
	    warning(WARNING_SEVERE, "Invalid \"%%%%DocumentMedia:\" line, insufficient parameters");
	    return;
	    }

	/* Make sure the things array is not full. */
	things_space_check();

	/*
	** Allocate space for and fill in a new
	** media record.
	*/
	newmedia = (struct Media*)gu_alloc(1,sizeof(struct Media));
	ASCIIZ_to_padded(newmedia->medianame,tokens[first],sizeof(newmedia->medianame));
	newmedia->width = gu_getdouble(tokens[first+1]);
	newmedia->height = gu_getdouble(tokens[first+2]);
	newmedia->weight = gu_getdouble(tokens[first+3]);
	ASCIIZ_to_padded(newmedia->colour,tokens[first+4],sizeof(newmedia->colour));
	ASCIIZ_to_padded(newmedia->type,tokens[first+5],sizeof(newmedia->type));

	/*
	** Set the next available things[] array entry
	** to point to our newly created media record.
	*/
	things[thing_count].th_ptr = (void*)newmedia;
	things[thing_count].th_type = TH_MEDIA;
	things[thing_count].R_Flags = MREF_DOC;
	thing_count++;
	} /* MREF_DOC */

    /* If page level media reference, */
    else if(reftype == MREF_PAGE)
	{
	char medianame[MAX_MEDIANAME];
	int x;
	int found=FALSE;

	ASCIIZ_to_padded(medianame,tokens[first],sizeof(medianame));

	for(x=0; x < thing_count; x++)		/* Look thru the whole thing list */
	    {
	    if(things[x].th_type != TH_MEDIA)	/* Ignore those that are not media */
		continue;

	    if(padded_cmp(medianame,((struct Media*)things[x].th_ptr)->medianame,MAX_MEDIANAME))
	    	{
		found = TRUE;
		things[x].R_Flags |= MREF_PAGE;
		set_thing_bit(x);
	    	}
	    }

	if( ! found )
	    warning(WARNING_SEVERE, "\"%%PageMedia:\" comment on page %d names undefined medium \"%s\"", pagenumber, tokens[first]);
	} /* MREF_PAGE */

    /* if unrecognized media reference type code */
    else
	{
	fatal(PPREXIT_OTHERERR, "ppr_media.c: media(): invalid reference type: %d", reftype);
	}
    } /* end of media() */

/*
** Dump media catalog for the whole document.
** That is, produce a "%%DocumentMedia:" comment.
**
** This write to ofile rather than directly to FILE *comments because
** if we split the job, ppr_split.c:split_job() will copy the comments
** file, leaving out all "%%DocumentMedia:" line and then call this
** function for each new -comments file.
*/
void dump_document_media(FILE *ofile, int frag)
    {
    int x;
    int number=0;
    struct Media *m;
    char medianame[sizeof(m->medianame)+1];
    char colour[sizeof(m->colour)+1];
    char type[sizeof(m->type)+1];

    for(x=0;x<thing_count;x++)
	{
	if(things[x].th_type==TH_MEDIA)
	    {
	    if( ! is_thing_in_current_fragment(x, frag) ) /* Ignore media if not in */
		continue;				/* this job fragment. */

	    if(number++)
		fprintf(ofile,"%%%%+ ");
	    else
		fprintf(ofile,"%%%%DocumentMedia: ");

	    m = (struct Media *) things[x].th_ptr;
	    padded_to_ASCIIZ(medianame,m->medianame,sizeof(m->medianame));
	    padded_to_ASCIIZ(colour,m->colour,sizeof(m->colour));
	    padded_to_ASCIIZ(type,m->type,sizeof(m->type));
	    fprintf(ofile,"%s %s ",medianame, gu_dtostr(m->width) );
	    fprintf(ofile,"%s ", gu_dtostr(m->height) );
	    fprintf(ofile,"%s %s ",gu_dtostr(m->weight),quote(colour));
	    fprintf(ofile,"%s\n",quote(type) );
	    }
	}
    } /* end of dump_document_media() */

/*
** Write a "%%PageMedia:" comment if necessary.
** This is called once for each page.
*/
void dump_page_media(void)
    {
    int x;
    struct Media *m;
    char medianame[sizeof(m->medianame)+1];

    for(x=0;x<thing_count;x++)
	{
	if( (things[x].th_type==TH_MEDIA) && (things[x].R_Flags & MREF_PAGE) )
	    {
	    m=(struct Media *) things[x].th_ptr;
	    padded_to_ASCIIZ(medianame,m->medianame,sizeof(m->medianame));

	    fprintf(page_comments, "%%%%PageMedia: %s\n", medianame);

	    things[x].R_Flags &= (~MREF_PAGE);
	    break;
	    }
	}
    } /* end of dump_page_media() */

/*
** Look one media up in the media database and write an appropriate
** "Media:" line into the queue file.
**
** This function is called by write_media_lines() below.
**
** If this function failes, it returns -1.  If it suceeds, it increments the
** number pointed to by match_count and returns 0.
**
** This function must always leave the media file at its start.
*/
static int write_media_line(FILE *qfile, FILE *mfile, struct Media *m, int *match_count)
    {
    struct Media km;				/* a known media record */
    char medianame[MAX_MEDIANAME+1];		/* ASCIIZ version of matched medium */
    char psmediumname[MAX_MEDIANAME+1];		/* ASCIIZ version of searched for medium */
    gu_boolean success = FALSE;

    #ifdef DEBUG_MEDIA_MATCHING
    printf("write_media_line()\n");
    #endif

    /* Make an ASCIIZ version of the media name from the PostScript comment. */
    padded_to_ASCIIZ(psmediumname, m->medianame, MAX_MEDIANAME);

    /*
    ** Search the media library file.
    ** Pass 1
    **
    ** On this try we will require that the width and height
    ** match exactly, that the weight match exactly or that
    ** the document not specify the weight, that the colour
    ** match exactly or the document not specify the colour
    ** and the actual medium colour be white, and that
    ** the medium type be an exact match.
    **
    ** Actually, the height and width no longer have to match
    ** exactly, they must be within 1 PSU (1/72th inch).  This
    ** is because Adobe has set an example of stating the size
    ** of A4 paper in round numbers.
    */
    while(fread(&km, sizeof(struct Media), 1, mfile))			/* 1st try */
	{
	#ifdef DEBUG_MEDIA_MATCHING
	padded_to_ASCIIZ(medianame,km.medianame,sizeof(km.medianame));
	printf("Trying medium \"%s\"\n", medianame);
	#endif

	if( FABS(m->height - km.height) < 1.0
			&& FABS(m->width - km.width) < 1.0		/* if height and width are correct */
		&& (m->weight==0                         		/* and weight unspecified */
		    || m->weight==km.weight)				/* or weight correct */
		&& ( (m->colour[0]==' '					/* and colour unspecified */
		    && strncmp("white ",km.colour,6)==0 )		/* matching white */
		    || padded_cmp(m->colour,km.colour,MAX_COLOURNAME) ) /* or correct */
		&& padded_cmp(m->type,km.type,MAX_TYPENAME) ) 		/* and type correct */
	    {
	    padded_to_ASCIIZ(medianame,km.medianame,			/* add this media */
			  sizeof(km.medianame));           		/* type */
	    fprintf(qfile, "Media: %s %s\n", medianame, psmediumname);  /* to queue file */

	    if(option_gab_mask & GAB_MEDIA_MATCHING)
		printf("Exact medium match: \"%s\" matches \"%s %.1f %.1f %.1f (%.*s) (%.*s)\"\n",
			psmediumname, medianame, km.width, km.height, km.weight, (int)strcspn(km.colour, " "), km.colour, (int)strcspn(km.type, " "), km.type);

	    (*match_count)++;                           		/* and set flag */
	    success = TRUE;
	    break;                                   			/* and stop search */
	    }
	}

    rewind(mfile);		/* return to begining of media file */
    if(success) return 0;	/* if we matched above, go to next media */

    /*
    ** Pass 2
    **
    ** We try again to find a match, this time with more liberal
    ** requirements.  This time the height and width must be close,
    ** the weight and colour are not considered, but the type must
    ** still be an exact match in all respects except case.
    */
    while( fread(&km,sizeof(struct Media), 1, mfile) )		/* 2nd try */
	{							/* anything close in size, and */
	if( abs( (int)(m->height - km.height) ) < 20		/* of the right type */
  			&& abs( (int)(m->width-km.width) ) < 20
  			&& padded_icmp(m->type, km.type, MAX_TYPENAME) )
	    {
	    padded_to_ASCIIZ(medianame, km.medianame, sizeof(km.medianame));
	    fprintf(qfile, "Media: %s %s\n", medianame, psmediumname);

	    warning(WARNING_SEVERE, _("Closest medium (\"%s %.1f %.1f %.1f (%.*s) (%.*s)\") is an\n"
		"\timperfect match for \"%s %.1f %.1f %.1f (%.*s) (%.*s)\""),
	    	medianame, km.width, km.height, km.weight, (int)strcspn(km.colour, " "), km.colour, (int)strcspn(km.type, " "), km.type,
	    	psmediumname, m->width, m->height, m->weight, (int)strcspn(m->colour, " "), m->colour, (int)strcspn(m->type, " "), m->type);

	    (*match_count)++;
	    success = TRUE;
	    break;
	    }
	}

    rewind(mfile);		/* return to begining of media file */
    if(success) return 0;	/* if matched above, continue with next media in document */

    /* Abort if the ProofMode is "NotifyMe". */
    if(qentry.attr.proofmode == PROOFMODE_NOTIFYME)
	{
    	ppr_abort(PPREXIT_NOMATCH, psmediumname);
    	}

    /*
    ** Pass 3
    **
    ** This is the final try.  We will be very careless.
    ** We will accept any paper without an explicit type (i. e., is blank)
    ** which has a size within half an inch of the desired size.
    */
    while(fread(&km, sizeof(struct Media), 1, mfile))
	{
	if( abs( (int)(m->height - km.height) ) < 36 && abs( (int)(m->width - km.width) ) < 36
  		&& km.type[0] == ' ' )
	    {
	    padded_to_ASCIIZ(medianame, km.medianame, sizeof(km.medianame));
	    fprintf(qfile, "Media: %s %s\n", medianame, psmediumname);

	    warning(WARNING_SEVERE, _("Closest medium (\"%s %.1f %.1f %.1f (%.*s) (%.*s)\") is a\n"
		"\tvery poor match for \"%s %.1f %.1f %.1f (%.*s) (%.*s)\""),
	    	medianame,
	    		km.width,
			km.height,
			km.weight,
			(int)strcspn(km.colour, " "), km.colour,
			(int)strcspn(km.type, " "), km.type,
	    	psmediumname,
			m->width,
			m->height,
			m->weight,
			(int)strcspn(m->colour, " "), m->colour,
			(int)strcspn(m->type, " "), m->type);

	    (*match_count)++;
	    success = TRUE;
	    break;
	    }
	}

    rewind(mfile);		/* return to begining of media file */
    if(success) return 0;	/* if matched above, skip to next loop iteration */
    return -1;
    } /* end of write_media_line() */

/*
** Write the "Media:" lines to the queue file.
** This procedure is called from write_queue_file().
**
** The "frag" parameter is used when we are splitting
** the job into multiple parts.  it is used in the call
** to is_thing_in_current_fragment().
*/
void write_media_lines(FILE *qfile, int frag)
    {
    FILE *mfile;
    int x;			/* used for moving through things[] */
    int match_count = 0;	/* "Media:" lines written due to matches */

    #ifdef DEBUG_MEDIA_MATCHING
    printf("write_media_lines()\n");
    #endif

    /* Open the media database file.  This contains a list of
       the media types which may be used with the "ppop media mount"
       command and a description of each. */
    if((mfile = fopen(MEDIAFILE, "rb")) == (FILE*)NULL)
	fatal(PPREXIT_OTHERERR, _("Can't open media database \"%s\", errno=%d (%s)"), MEDIAFILE, errno, gu_strerror(errno));

    /* For each of the "things" which are media, write
       a "Media:" line to the queue file by calling
       write_media_line(). */
    for(x=0; x < thing_count; x++)
	{
	#ifdef DEBUG_MEDIA_MATCHING
	printf("x = %d\n", x);
	#endif

	if(things[x].th_type != TH_MEDIA)	/* Ignore things which are */
	    continue;				/* are not media. */

	if( ! is_thing_in_current_fragment(x, frag) )	/* Ignore media not in */
	    continue;					/* this job fragment. */

	if(write_media_line(qfile, mfile, (struct Media *)things[x].th_ptr, &match_count))
	    warning(WARNING_SEVERE, _("Requested medium is not listed in the media database"));

	#ifdef DEBUG_MEDIA_MATCHING
	printf("match_count = %d\n", match_count);
	#endif
	} /* end of for each `thing' */

    /*
    ** If no %%DocumentMedia: comments resulted in matches, use the default
    ** medium as modified by information gathered from Feature comments
    ** in the document setup section.
    **
    ** If the document selects
    ** a radically different page size, we could end up with
    ** something quite different.  For example, if the default
    ** medium is "letter" and there is a comment in the document
    ** which indicates that "a4" pagesize is selected, we will end
    ** up selecting the medium "a4" if it exists in the media
    ** database.
    */
    if(match_count == 0)	/* if no valid media found */
	{			/* use the default media adjusted to PageSize etc. */
	struct Media def;	/* for reading media */
	gu_boolean modified = FALSE;

	if(option_gab_mask & GAB_MEDIA_MATCHING)
	    printf("Resorting to default medium: %s\n", default_medium ? default_medium : "???");

	/* If -D switch on command line, */
	if(default_medium)
            {
            char padded_default[MAX_MEDIANAME];
            ASCIIZ_to_padded(padded_default, default_medium, MAX_MEDIANAME);
            while(TRUE)
                {
                if(fread(&def, sizeof(struct Media), 1, mfile) == 0)        /* if end of file, */
                    fatal(PPREXIT_NOMATCH, _("default medium \"%s\" does not exist"), default_medium);

                if(memcmp(def.medianame, padded_default, MAX_MEDIANAME) == 0)
                    break;
                } /* end of loop to find the default media */
            }

	/* If no -D switch, use ppr.conf */
	else
	    {
            const char file[] = PPR_CONF;
            const char section[] = "internationalization";
            const char key[] = "defaultmedium";
            const char *error_message;
	    char *MediaColor;
	    char *MediaType;

            error_message = gu_ini_scan_list(file, section, key,
                GU_INI_TYPE_SKIP,
                GU_INI_TYPE_POSITIVE_DOUBLE, &def.width,
                GU_INI_TYPE_POSITIVE_DOUBLE, &def.height,
                GU_INI_TYPE_NONNEG_DOUBLE, &def.weight,
                GU_INI_TYPE_STRING, &MediaColor,
                GU_INI_TYPE_STRING, &MediaType,
                GU_INI_TYPE_END);

            if(error_message)
                {
                fatal(PPREXIT_CONFIG, _("%s\n"
                                "\twhile attempting to read \"%s\"\n"
                                "\t\t[%s]\n"
                                "\t\t%s =\n"),
                        gettext(error_message), file, section, key);
                }

	    ASCIIZ_to_padded(def.colour, MediaColor, MAX_COLOURNAME);
	    gu_free(MediaColor);
	    ASCIIZ_to_padded(def.type, MediaType, MAX_TYPENAME);
	    gu_free(MediaType);
	    }

        /*
        ** Change the `DSC comment name' (which will be the first
        ** field of the "%%DocumentMedia:" line) to "NULL" and
        ** break out of this loop.
        **
        ** There are a number of reasons we change it to "NULL", not
        ** the least of these being that once we are thru adjusting
        ** the page size and such we may not select that
        ** medium at all.
        */
        ASCIIZ_to_padded(def.medianame, "NULL", MAX_MEDIANAME);

	/*
	** If *PageSize or *PageRegion is given us a width,
	** patch the default medium structure using information
	** we have gathered in guess_media.
	*/
	if(guess_media.width != 0.0 && guess_media.height != 0.0
		&& (guess_media.width != def.width || guess_media.height != def.height) )
	    {
	    if(option_gab_mask & GAB_MEDIA_MATCHING)
		printf("Default medium size overridden: %.1f x %.1f -> %.1f x %.1f\n", def.width, def.height, guess_media.width, guess_media.height);
	    def.width = guess_media.width;
	    def.height = guess_media.height;
	    modified = TRUE;
	    }

	/* *MediaWeight */
	if(guess_media.weight != 0.0 && guess_media.weight != def.weight)
	    {
	    if(option_gab_mask & GAB_MEDIA_MATCHING)
	    	printf("Default media weight overriden: %.1f -> %.1f\n", def.weight, guess_media.weight);
	    def.weight = guess_media.weight;
	    modified = TRUE;
	    }

	/* *MediaColor */
	if(guess_media.colour[0] && memcmp(guess_media.colour, def.colour, MAX_COLOURNAME))
	    {
	    if(option_gab_mask & GAB_MEDIA_MATCHING)
		printf("Default media colour overridden: \"%.*s\" -> \"%.*s\"\n", (int)strcspn(def.colour, " "), def.colour, (int)strcspn(guess_media.colour, " "), guess_media.colour);
	    memcpy(def.colour,guess_media.colour,MAX_COLOURNAME);
	    modified = TRUE;
	    }

	/* if "*MediaType" or "*PageSize Comm10", etc. */
	if(guess_media.type[0] && memcmp(guess_media.type,def.type,MAX_TYPENAME))
	    {
	    if(option_gab_mask & GAB_MEDIA_MATCHING)
	    	printf("Default media type overriden: \"%.*s\" -> \"%.*s\"\n", (int)strcspn(def.type, " "), def.type, (int)strcspn(guess_media.type, " "), guess_media.type);
   	    memcpy(def.type, guess_media.type, MAX_TYPENAME);
	    modified = TRUE;
	    }

	rewind(mfile);
	if(write_media_line(qfile, mfile, &def, &match_count))
	    {
	    warning(WARNING_SEVERE, _("Default medium as overridden (\"%.1f %.1f %.1f (%.*s) (%.*s)\")\n"
	    		"\tdoes not exist, will call for fictitious medium \"UNRECOGNIZED\""),
	    	def.width, def.height, def.weight, (int)strcspn(def.colour, " "), def.colour, (int)strcspn(def.type, " "), def.type);

	    /* This is desparate code. */
	    fprintf(qfile, "Media: UNRECOGNIZED NULL\n");
	    }
	} /* end of if(no match) then, use default medium */

    fclose(mfile);              /* close the media library */

    #ifdef DEBUG_MEDIA_MATCHING
    printf("write_media_lines(): done\n");
    #endif

    } /* end of write_media_lines() */

/* end of file */

