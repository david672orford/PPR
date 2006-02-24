/*
** mouse:~ppr/src/ppad/ppad_media.c
** Copyright 1995--2006, Trinity College Computing Center.
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
** Last modified 23 February 2006.
*/

/*============================================================================
** This module is part of ppad(8), PPR's administration program for
** PostScript page printers.  This module contains the media database
** management routines.
<helptopic>
	<name>media</name>
	<desc>all settings for media list</desc>
</helptopic>
============================================================================*/

#include "config.h"
#include <unistd.h>
#include <string.h>
#include <memory.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "global_structs.h"
#include "util_exits.h"
#include "ppad.h"
#include "dispatch_table.h"

/* globals */
static int inerror = 0;			/* used by get_answer() */

/*
** get an answer from the user or command line
*/
static void get_answer(char *buffer, int len, const char *argv[], int *index)
	{
	if(!len)
		fatal(EXIT_INTERNAL, "get_answer(): len is zero");

	if(argv[*index])
		{
		if(inerror)								/* prepared input */
			exit(1);							/* can't correct errors */
		strncpy(buffer, argv[(*index)++], len-1);
		buffer[len-1] = '\0';
		gu_utf8_printf("%s\n", buffer);
		}
	else
		{
		fgets(buffer, len, stdin);
		buffer[strcspn(buffer,"\n")] = '\0';
		}
	} /* end of get_answer() */

/*
** A wrapper around convert_dimension() which prints an error message
** if it fails.
*/
static double ppad_convert_dimension(const char *string)
	{
	double answer;

	if((answer = convert_dimension(string)) < 0)
		{
		gu_utf8_fputs(_("Unknown unit specifier.\n"), stderr);
		return -1;
		}

	return answer;
	} /* end of ppad_convert_dimension */

/*
** Open the media database, creating it first if it does not already exist.
*/
static FILE *open_database(const char mode[])
	{
	FILE *f;
	int i = 1;

	while(TRUE)
		{
		if((f = fopen(MEDIAFILE, "r+b")) == (FILE*)NULL)
			{
			if(errno == ENOENT && i-- > 0)
				{
				int fd;
				gu_utf8_printf(_("Creating media file \"%s\".\n"), MEDIAFILE);
				if((fd = open(MEDIAFILE, O_WRONLY | O_CREAT, UNIX_644)) == -1)
					fatal(EXIT_INTERNAL, _("Can't create media file \"%s\", errno=%d (%s)"), MEDIAFILE, errno, gu_strerror(errno));
				close(fd);
				continue;
				}
			else
				{
				fatal(EXIT_INTERNAL, _("Can't open media database \"%s\", errno=%d (%s)"), MEDIAFILE, errno, gu_strerror(errno));
				}
			}
		break;
		}

	return f;
	}

/*
<command acl="ppad" helptopic="media">
	<name><word>media</word><word>put</word></name>
	<desc>add or modify a media database record</desc>
	<args>
		<arg flags="optional"><name>medium</name><desc>name of medium</desc></arg>
		<arg flags="optional"><name>width</name><desc>width of medium</desc></arg>
		<arg flags="optional"><name>height</name><desc>height of medium</desc></arg>
		<arg flags="optional"><name>weight</name><desc>weight of medium</desc></arg>
		<arg flags="optional"><name>color</name><desc>color of medium</desc></arg>
		<arg flags="optional"><name>suitability</name><desc>suitability as flag page (1-10)</desc></arg>
	</args>
</command>
*/
int command_media_put(const char *argv[])
	{
	FILE *ffile;
	struct Media media;
	char asciiz[sizeof(media.medianame)+2]; /* ASCIIZ version of a string */
	char padded[sizeof(media.medianame)];	/* space padded version of same */
	int exists=0;							/* set true if already exists */
	int index=0;

	/* get the name of the medium to be added or changed */
	gu_utf8_printf(_("Medium Name: "));
	get_answer(asciiz,sizeof(asciiz),argv,&index);
	ASCIIZ_to_padded(padded,asciiz,sizeof(padded));

	ffile = open_database("r+b");

	/* look for the media name in the database */
	while(fread(&media,sizeof(struct Media),1,ffile) != 0)
		{
		if(memcmp(media.medianame,padded,sizeof(media.medianame))==0)
			{
			exists = -1;
			break;
			}
		}

	/* store the medianame */
	ASCIIZ_to_padded(media.medianame,asciiz,sizeof(media.medianame));

	/* width */
	gu_utf8_printf("\n");
	gu_utf8_printf(_("Accurate width is required.\n"));
	do	{
		double x;
		if(exists)
			{
			gu_utf8_printf(_("Width: (%.2f in, %.1f pt, %.1f cm) "),
							media.width/72.0,
							media.width,
							media.width / 72.0 * 2.54 );
			get_answer(asciiz,sizeof(asciiz),argv,&index);
			if(*asciiz)
				{
				x=ppad_convert_dimension(asciiz);
				if(x==-1)
					inerror=-1;
				else
					{
					inerror=0;
					media.width=x;
					}
				}
			else
				inerror=0;
			}
		else
			{
			do	{
				gu_utf8_printf(_("Width: "));
				get_answer(asciiz,sizeof(asciiz),argv,&index);
				} while(*asciiz == '\0');
			x=ppad_convert_dimension(asciiz);
			if(x==-1)
				inerror=-1;
			else
				{
				inerror=0;
				media.width=x;
				}
			}
		} while(inerror);

	/* height */
	gu_utf8_printf("\n");
	gu_utf8_printf(_("Accurate height is required.\n"));
	do	{
		double x;
		if(exists)
			{
			gu_utf8_printf(_("Height: (%.2f in, %.1f pt, %.1f cm) "),
							media.height/72.0,
							media.height,
							media.height / 72.0 * 2.54 );
			get_answer(asciiz,sizeof(asciiz),argv,&index);
			if(*asciiz)
				{
				x=ppad_convert_dimension(asciiz);	 /* convert to points */
				if(x==-1)						/* if couldn't convert */
					inerror=-1;					/* we have an error */
				else
					{							/* if could convert */
					inerror=0;					/* clear error */
					media.height=x;				/* and use the figure */
					}
				}
			else								/* accepting present value */
				inerror=0;						/* is not possibly an error */
			}
		else								/* does not already exist */
			{
			do	{							/* keep asking until answered */
				gu_utf8_printf(_("Height: "));
				get_answer(asciiz,sizeof(asciiz),argv,&index);
				} while(*asciiz == '\0');
			x=ppad_convert_dimension(asciiz);	 /* convert to points */
			if(x==-1)						/* if conversion failed */
				inerror=-1;					/* set error flag */
			else
				{							/* if didn't fail */
				inerror=0;					/* clear error flag */
				media.height=x;				/* and use the number */
				}
			}
		} while(inerror);

	/* weight */
	gu_utf8_printf("\n");
	gu_utf8_printf(_("If weight is unknown, enter \"0\".\n"));
	if(exists)
		{
		gu_utf8_printf(_("Weight: (%.1f grams per square metre) "), media.weight);
		get_answer(asciiz,sizeof(asciiz),argv,&index);
		if(*asciiz)
			media.weight = atoi(asciiz);
		}
	else
		{
		do	{
			gu_utf8_printf(_("Weight (grams per square metre): "));
			get_answer(asciiz,sizeof(asciiz), argv, &index);
			inerror = -1;
			} while(*asciiz == '\0');
		media.weight = gu_getdouble(asciiz);
		}
	inerror=0;

	/* Ask for the PostScript colour name. */
	gu_utf8_printf("\n");
	gu_utf8_printf(_("Colour must be specified.\n"));
	padded_to_ASCIIZ(asciiz, media.colour, sizeof(media.colour));
	if(exists && *asciiz)
		{
		gu_utf8_printf(_("Colour: (%s) "), asciiz);
		get_answer(asciiz, sizeof(asciiz), argv, &index);
		if(*asciiz)
			{
			ASCIIZ_to_padded(media.colour,asciiz,sizeof(media.colour));
			}
		}
	else			   /* If new entry */
		{			   /* insist on an answer */
		do	{
			gu_utf8_printf(_("Colour: "));
			get_answer(asciiz, sizeof(asciiz), argv, &index);
			} while(*asciiz == '\0');
		ASCIIZ_to_padded(media.colour,asciiz,sizeof(media.colour));
		}

	/* read the PostScript type */
	gu_utf8_printf("\n");
	gu_utf8_printf(_("Recomended types are:	 19HoleCerlox, 3Hole, 2Hole,\n"
		"\tColorTransparency, CorpLetterHead, CorpLogo,\n"
		"\tCustLetterHead, DeptLetterHead, Labels, Tabs\n"
		"\tTransparency, and UserLetterHead\n"
		"This field should be left blank for ordinary blank paper.\n"
		"Enter \"()\" to delete Type.\n"));
	padded_to_ASCIIZ(asciiz, media.type, sizeof(media.type));
	if( exists && *asciiz )
		{
		gu_utf8_printf(_("Pre-printed Form Type: (%s) "), asciiz);
		get_answer(asciiz, sizeof(asciiz), argv, &index);
		if(*asciiz)
			{
			if(strcmp(asciiz,"()")==0)			/* "()" is blank */
				ASCIIZ_to_padded(media.type,"",sizeof(media.type));
			else
				ASCIIZ_to_padded(media.type,asciiz,sizeof(media.type));
			}
		}
	else
		{
		gu_utf8_printf(_("Pre-printed Form Type: "));
		get_answer(asciiz,sizeof(asciiz),argv,&index);
		ASCIIZ_to_padded(media.type,asciiz,sizeof(media.type));
		}

	/* get suitability for banners and trailers */
	gu_utf8_printf("\n");
	gu_utf8_printf(_("Rank on a scale of 1 to 10, with 1 being entirely unsuitable.\n"));
	if(exists)
		{
		#ifdef GNUC_HAPPY
		int t=0;
		#else
		int t;
		#endif
		do	{
			gu_utf8_printf(_("Suitability for banners and trailers: (%d) "), media.flag_suitability);
			get_answer(asciiz,sizeof(asciiz),argv,&index);
			} while( *asciiz && (t=atoi(asciiz)) < 1 && t > 10 );
		if(*asciiz)		/* if something entered */
			media.flag_suitability=t;
		}
	else				/* new entry */
		{
		int t;
		do	{
			gu_utf8_printf(_("Suitability for banners and trailers: "));
			get_answer(asciiz,sizeof(asciiz),argv,&index);
			} while( (t=atoi(asciiz)) < 1 || t > 10 );
		media.flag_suitability=t;
		}

	fflush(ffile);								/* flush before write */
	if(exists)									/* if exists */
		{										/* go back to orig */
		fseek(ffile, (long int) (0 - sizeof(struct Media)), SEEK_CUR);
		}				/* (new media gets written at end of file */
	fwrite(&media,sizeof(struct Media),1,ffile);
	fclose(ffile);

	return EXIT_OK;
	} /* end of media_put() */

/*
<command helptopics="media">
	<name><word>media</word><word>show</word></name>
	<desc>display a media database record</desc>
	<args>
		<arg flags="optional"><name>medium</name><desc>name of medium</desc></arg>
	</args>
</command>
*/
int command_media_show(const char *argv[])
	{
	int index = 0;				/* moving index into argv[] for answers */
	FILE *ffile;
	struct Media media;
	char asciiz[sizeof(media.medianame)+2];
	char padded[sizeof(media.medianame)];
	int all;
	int ret = EXIT_OK;

	gu_utf8_printf(_("Medium Name: "));
	get_answer(asciiz, sizeof(asciiz), argv, &index);

	if(strcmp(asciiz, "all") == 0)
		{
		all = TRUE;
		}
	else
		{
		ASCIIZ_to_padded(padded,asciiz, sizeof(padded));
		all = FALSE;
		}

	ffile = open_database("rb");

	while(TRUE)
		{
		if(fread(&media,sizeof(struct Media),1,ffile) == 0)
			{
			if(! all)
				{
				gu_utf8_fprintf(stderr, _("Medium \"%s\" not found.\n"), asciiz);
				ret = EXIT_NOTFOUND;
				}
			break;
			}
		if( all || (memcmp(media.medianame,padded,sizeof(media.medianame))==0) )
			{
			gu_utf8_printf("\n");

			if(all)				/* if displaying all, the medium name is not obvious */
				gu_utf8_printf(_("Medium Name: %16.16s\n"), media.medianame);

			gu_utf8_printf(_("Width: %.2f inches, %.1f points, %.1f centimetres\n"),
									media.width/72.0,
									media.width,
									media.width/72.0*2.54 );
			gu_utf8_printf(_("Height: %.2f inches, %.1f points, %.1f centimetres\n"),
									media.height/72.0,
									media.height,
									media.height/72.0*2.54 );
			gu_utf8_printf(_("Weight: %.1f grams per square metre\n"), media.weight);
			gu_utf8_printf(_("Colour: %16.16s\n"), media.colour);
			gu_utf8_printf(_("Pre-printed Form Type: %16.16s\n"), media.type);
			gu_utf8_printf(_("Banner/Trailer suitability: %d\n"), media.flag_suitability);

			if( ! all ) break;
			}
		}

	if(fclose(ffile) == EOF)
		fatal(EXIT_INTERNAL, "media_show(): fclose() failed, errno=%d (%s)", errno, gu_strerror(errno));

	return ret;
	} /* end of media_show() */

/*
<command acl="ppad" helptopics="media">
	<name><word>media</word><word>delete</word></name>
	<desc>delete a media database record</desc>
	<args>
		<arg flags="optional"><name>medium</name><desc>name of medium</desc></arg>
	</args>
</command>
*/
int command_media_delete(const char *argv[])
	{
	int index=0;
	FILE *ffile;
	struct Media media;
	char asciiz[sizeof(media.medianame)+2];
	char padded[sizeof(media.medianame)];

	gu_utf8_printf(_("Media Name: "));
	get_answer(asciiz,sizeof(asciiz),argv,&index);
	ASCIIZ_to_padded(padded,asciiz,sizeof(padded));

	ffile = open_database("r+b");

	while(TRUE)
		{
		if(fread(&media,sizeof(struct Media),1,ffile) == 0)
			{
			fclose(ffile);
			gu_utf8_fprintf(stderr, _("Medium \"%s\" not found.\n"), asciiz);
			return EXIT_NOTFOUND;
			}
		if(memcmp(media.medianame,padded,sizeof(media.medianame))==0)
			{					/* if entry to be deleted found */
			while( fread(&media,sizeof(struct Media),1,ffile) == 1 )
				{				/* read the next record */
				fflush(ffile);
				fseek(ffile, (long int) (0 - (sizeof(struct Media)*2)), SEEK_CUR);
				fwrite(&media,sizeof(struct Media),1,ffile);  /* step back 2 records and write */
				fflush(ffile);
				fseek(ffile, (long int)sizeof(struct Media), SEEK_CUR);
				}				/* skip over orig of record previously copied */

			ftruncate(fileno(ffile), (long int)(ftell(ffile)-sizeof(struct Media)));
			fclose(ffile);
			gu_utf8_printf(_("Medium deleted.\n"));
			return EXIT_OK;
			}
		}
	} /* end of media_delete() */

/*
<command helptopics="media">
	<name><word>media</word><word>export</word></name>
	<desc>emmit a shell script which can be used to recreate the media database</desc>
	<args>
	</args>
</command>
*/
int command_media_export(const char *argv[])
	{
	FILE *ffile;
	struct Media media;
	char name[sizeof(media.medianame)+1];
	char colour[sizeof(media.colour)+1];
	char type[sizeof(media.type)+1];

	ffile = open_database("rb");

	gu_utf8_putline("#! /bin/sh");

	while(fread(&media,sizeof(struct Media),1,ffile) == 1 )
		{
		padded_to_ASCIIZ(name,media.medianame,sizeof(media.medianame));
		padded_to_ASCIIZ(colour,media.colour,sizeof(media.colour));
		padded_to_ASCIIZ(type,media.type,sizeof(media.type));

		gu_utf8_printf("ppad media put \"%s\" \"%.1f pt\" \"%.1f pt\" %.1f \"%s\" \"%s\" %d\n",
				name,
				media.width,
				media.height,
				media.weight,
				colour,
				type,
				media.flag_suitability);
		}
	fclose(ffile);
	return EXIT_OK;
	} /* end of media_export() */

/*
<command acl="ppad" helptopic="media">
	<name><word>media</word><word>import</word></name>
	<desc>emmit a shell script which can be used to recreate the media database</desc>
	<args>
		<arg><name>filename</name><desc>name of file to import</desc></arg>
	</args>
</command>
*/
int command_media_import(const char *argv[])
	{
	const char *filename = argv[0];
	FILE *f;
	int line_available = 80;
	char *line = NULL;
	int linenum = 0;
	#define MAX_CMD_WORDS 64
	const char *ar[MAX_CMD_WORDS+1];	/* argument vector constructed from line[] */
	char *p;							/* used to parse arguments */
	int i;

	if(strcmp(filename, "-") == 0)
		{
		f = fdopen(dup(0), "r");
		}
	else
		{
		if(!(f = fopen(filename, "r")))
			{
			gu_utf8_fprintf(stderr, _("Can't open \"%s\", errno=%d (%s)\n"), filename, errno, gu_strerror(errno));
			return EXIT_NOTFOUND;
			}
		}

	while((p = line = gu_getline(line, &line_available, f)))
		{
		linenum++;

		if(line[0] == '#' || line[0] == ';' || strlen(line) == 0)
			continue;

		for(i=0; (ar[i] = gu_strsep_quoted(&p, " \t\n", NULL)); i++)
			{
			if(i == MAX_CMD_WORDS)
				{
				gu_utf8_putline(X_("Warning: command buffer overflow!"));	/* temporary code, don't internationalize */
				ar[i] = NULL;
				break;
				}
			}

		if(ar[0] && gu_strcasecmp(ar[0], "ppad") == 0
				&& ar[1] && gu_strcasecmp(ar[1], "media") == 0
				&& ar[2] && gu_strcasecmp(ar[2], "put") == 0
				)
			{
			int ret = command_media_put(&ar[3]);
			if(ret != EXIT_OK)
				return ret;
			}
		else
			{
			gu_utf8_fprintf(stderr, _("Command on line %d is not ppad media put.\n"), linenum);
			break;
			}
		}

	if(line)
		{
		gu_free(line);
		return EXIT_SYNTAX;
		}

	return EXIT_OK;
	} /* command_media_export() */

/* end of file */
