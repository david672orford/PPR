/*
** mouse:~ppr/src/ppad/ppad_media.c
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
** Last modified 6 August 2003.
*/

/*
** This module is part of ppad(8), PPR's administration program for PostScript
** page printers.  This module contains the media database management routines.
*/

#include "before_system.h"
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
		printf("%s\n", buffer);
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

	if((answer=convert_dimension(string)) < 0)
		{
		fputs(_("Unknown unit specifier.\n"), errors);
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
				printf(_("Creating media file \"%s\".\n"), MEDIAFILE);
				if((fd = open(MEDIAFILE, O_WRONLY | O_CREAT, UNIX_644)) == -1)
					fatal(EXIT_INTERNAL, _("Can't create media file \"%s\", errno=%d (%s)"), MEDIAFILE, errno, gu_strerror(errno));
				close(fd);
				continue;
				}
			else
				{
				fatal(EXIT_INTERNAL, _("Can't open media file \"%s\", errno=%d (%s)"), MEDIAFILE, errno, gu_strerror(errno));
				}
			}
		break;
		}

	return f;
	}

/*
** add or modify a media database record
*/
int media_put(const char *argv[])
	{
	FILE *ffile;
	struct Media media;
	char asciiz[sizeof(media.medianame)+2]; /* ASCIIZ version of a string */
	char padded[sizeof(media.medianame)];	/* space padded version of same */
	int exists=0;							/* set true if already exists */
	int index=0;

	if( ! am_administrator() )
		return EXIT_DENIED;

	/* get the name of the medium to be added or changed */
	printf(_("Medium Name: "));
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
	printf("\n");
	printf(_("Accurate width is required.\n"));
	do	{
		double x;
		if(exists)
			{
			printf(_("Width: (%.2f in, %.1f pt, %.1f cm) "),
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
				printf(_("Width: "));
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
	printf("\n");
	printf(_("Accurate height is required.\n"));
	do	{
		double x;
		if(exists)
			{
			printf(_("Height: (%.2f in, %.1f pt, %.1f cm) "),
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
				printf(_("Height: "));
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
	printf("\n");
	printf(_("If weight is unknown, enter \"0\".\n"));
	if(exists)
		{
		printf(_("Weight: (%.1f grams per square metre) "), media.weight);
		get_answer(asciiz,sizeof(asciiz),argv,&index);
		if(*asciiz)
			media.weight = atoi(asciiz);
		}
	else
		{
		do	{
			printf(_("Weight (grams per square metre): "));
			get_answer(asciiz,sizeof(asciiz), argv, &index);
			inerror = -1;
			} while(*asciiz == '\0');
		media.weight = gu_getdouble(asciiz);
		}
	inerror=0;

	/* Ask for the PostScript colour name. */
	printf("\n");
	printf(_("Colour must be specified.\n"));
	padded_to_ASCIIZ(asciiz, media.colour, sizeof(media.colour));
	if(exists && *asciiz)
		{
		printf(_("Colour: (%s) "), asciiz);
		get_answer(asciiz, sizeof(asciiz), argv, &index);
		if(*asciiz)
			{
			ASCIIZ_to_padded(media.colour,asciiz,sizeof(media.colour));
			}
		}
	else			   /* If new entry */
		{			   /* insist on an answer */
		do	{
			printf(_("Colour: "));
			get_answer(asciiz, sizeof(asciiz), argv, &index);
			} while(*asciiz == '\0');
		ASCIIZ_to_padded(media.colour,asciiz,sizeof(media.colour));
		}

	/* read the PostScript type */
	printf("\n");
	printf(_("Recomended types are:	 19HoleCerlox, 3Hole, 2Hole,\n"
		"\tColorTransparency, CorpLetterHead, CorpLogo,\n"
		"\tCustLetterHead, DeptLetterHead, Labels, Tabs\n"
		"\tTransparency, and UserLetterHead\n"
		"This field should be left blank for ordinary blank paper.\n"
		"Enter \"()\" to delete Type.\n"));
	padded_to_ASCIIZ(asciiz, media.type, sizeof(media.type));
	if( exists && *asciiz )
		{
		printf(_("Preprinted Form Type: (%s) "), asciiz);
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
		printf(_("Preprinted Form Type: "));
		get_answer(asciiz,sizeof(asciiz),argv,&index);
		ASCIIZ_to_padded(media.type,asciiz,sizeof(media.type));
		}

	/* get suitability for banners and trailers */
	printf("\n");
	printf(_("Rank on a scale of 1 to 10, with 1 being entirely unsuitable.\n"));
	if(exists)
		{
		#ifdef GNUC_HAPPY
		int t=0;
		#else
		int t;
		#endif
		do	{
			printf(_("Suitability for banners and trailers: (%d) "), media.flag_suitability);
			get_answer(asciiz,sizeof(asciiz),argv,&index);
			} while( *asciiz && (t=atoi(asciiz)) < 1 && t > 10 );
		if(*asciiz)		/* if something entered */
			media.flag_suitability=t;
		}
	else				/* new entry */
		{
		int t;
		do	{
			printf(_("Suitability for banners and trailers: "));
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
** display a media record
*/
int media_show(const char *argv[])
	{
	int index = 0;				/* moving index into argv[] for answers */
	FILE *ffile;
	struct Media media;
	char asciiz[sizeof(media.medianame)+2];
	char padded[sizeof(media.medianame)];
	int all;
	int ret = EXIT_OK;

	printf(_("Medium Name: "));
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
				fprintf(errors, _("Medium \"%s\" not found.\n"), asciiz);
				ret = EXIT_NOTFOUND;
				}
			break;
			}
		if( all || (memcmp(media.medianame,padded,sizeof(media.medianame))==0) )
			{
			printf("\n");

			if(all)				/* if displaying all, the medium name is not obvious */
				printf(_("Medium Name: %16.16s\n"), media.medianame);

			printf(_("Width: %.2f inches, %.1f points, %.1f centimetres\n"),
									media.width/72.0,
									media.width,
									media.width/72.0*2.54 );
			printf(_("Height: %.2f inches, %.1f points, %.1f centimetres\n"),
									media.height/72.0,
									media.height,
									media.height/72.0*2.54 );
			printf(_("Weight: %.1f grams per square metre\n"), media.weight);
			printf(_("Colour: %16.16s\n"), media.colour);
			printf(_("Form Type: %16.16s\n"), media.type);
			printf(_("Banner/Trailer suitability: %d\n"), media.flag_suitability);

			if( ! all ) break;
			}
		}

	if(fclose(ffile) == EOF)
		fatal(EXIT_INTERNAL, "media_show(): fclose() failed, errno=%d (%s)", errno, gu_strerror(errno));

	return ret;
	} /* end of media_show() */

int media_delete(const char *argv[])
	{
	int index=0;
	FILE *ffile;
	struct Media media;
	char asciiz[sizeof(media.medianame)+2];
	char padded[sizeof(media.medianame)];

	printf(_("Media Name: "));
	get_answer(asciiz,sizeof(asciiz),argv,&index);
	ASCIIZ_to_padded(padded,asciiz,sizeof(padded));

	ffile = open_database("r+b");

	while(TRUE)
		{
		if(fread(&media,sizeof(struct Media),1,ffile) == 0)
			{
			fclose(ffile);
			fprintf(errors, _("Medium \"%s\" not found.\n"), asciiz);
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
			printf(_("Medium deleted.\n"));
			return EXIT_OK;
			}
		}
	} /* end of media_delete() */

/*
** Emmit a shell script which could be used to recreate the database.
*/
int media_export(void)
	{
	FILE *ffile;
	struct Media media;
	char name[sizeof(media.medianame)+1];
	char colour[sizeof(media.colour)+1];
	char type[sizeof(media.type)+1];

	ffile = open_database("rb");

	puts("#! /bin/sh");

	while(fread(&media,sizeof(struct Media),1,ffile) == 1 )
		{
		padded_to_ASCIIZ(name,media.medianame,sizeof(media.medianame));
		padded_to_ASCIIZ(colour,media.colour,sizeof(media.colour));
		padded_to_ASCIIZ(type,media.type,sizeof(media.type));

		printf("ppad media put \"%s\" \"%.1f pt\" \"%.1f pt\" %.1f \"%s\" \"%s\" %d\n",
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
** Import a file of ppad media commands.
*/
int media_import(const char *argv[])
	{
	const char *filename;
	FILE *f;
	int line_available = 80;
	char *line = NULL;
	int linenum = 0;
	#define MAX_CMD_WORDS 64
	const char *ar[MAX_CMD_WORDS+1];	/* argument vector constructed from line[] */
	char *p;							/* used to parse arguments */
	int i;

	if( ! am_administrator() )
		return EXIT_DENIED;

	if(!(filename = argv[0]))
		{
		fputs(_("You must supply a filename.\n"), errors);
		return EXIT_SYNTAX;
		}

	if(strcmp(filename, "-") == 0)
		{
		f = fdopen(dup(0), "r");
		}
	else
		{
		if(!(f = fopen(filename, "r")))
			{
			fprintf(errors, _("Can't open \"%s\", errno=%d (%s)\n"), filename, errno, gu_strerror(errno));
			return EXIT_NOTFOUND;
			}
		}

	while((p = line = gu_getline(line, &line_available, f)))
		{
		linenum++;

		if(line[0] == '#' || strlen(line) == 0)
			continue;

		for(i=0; (ar[i] = gu_strsep_quoted(&p, " \t\n", NULL)); i++)
			{
			if(i == MAX_CMD_WORDS)
				{
				puts(X_("Warning: command buffer overflow!"));	/* temporary code, don't internationalize */
				ar[i] = NULL;
				break;
				}
			}

		if(ar[0] && gu_strcasecmp(ar[0], "ppad") == 0
				&& ar[1] && gu_strcasecmp(ar[1], "media") == 0
				&& ar[2] && gu_strcasecmp(ar[2], "put") == 0
				)
			{
			media_put(&ar[3]);
			}
		else
			{
			fprintf(errors, _("Command on line %d is not ppad media put.\n"), linenum);
			break;
			}
		}

	if(line)
		{
		gu_free(line);
		return EXIT_SYNTAX;
		}

	return EXIT_OK;
	}

/* end of file */
