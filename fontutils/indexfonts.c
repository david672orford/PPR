/*
** mouse:~ppr/src/fontutils/indexfonts.c
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
** Last modified 5 November 2003.
*/

#include "before_system.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#ifdef INTERNATIONAL
#include <locale.h>
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "util_exits.h"
#include "libttf.h"
#include "libppr_font.h"
#include "version.h"

const char myname[] = "lib/indexfonts";
const char ppr_conf[] = PPR_CONF;
const char section_name[] = "fonts";
const char fontindex_db[] = FONT_INDEX;

static void indent(int i)
	{
	int x;
	for(x=0; x<i; x++)
		{
		fputc(' ', stdout);
		}
	}

/*==================================================================
** These functions open a Truetype font file and get its
** PostScript name.
==================================================================*/
static struct FONT_INFO *study_truetype_font(const char filename[])
	{
	void *font;
	char *psname;
	struct FONT_INFO *p;

	/* Open the truetype file and return an object pointer. */
	{
	TTF_RESULT ttf_result;
	if((ttf_result = ttf_new(&font, filename)) != TTF_OK)
		{
		fprintf(stderr, "%s: ttf_new() failed while processing %s, \"%s\"\n", myname, filename, ttf_strerror(ttf_result));
		return NULL;
		}
	}

	/* Extract the PostScript name. */
	if((psname = ttf_get_psname(font)) == NULL)
		{
		/*fprintf(stderr, "%s: ttf_get_psname() failed while processing \"%s\", %s\n", myname, filename, ttf_strerror(ttf_errno(font)));*/
		printf(" -- no PostScript name");
		return NULL;
		}

	/* Copy the name before ttf_delete() destroys it. */
	psname = gu_strdup(psname);

	/* Destroy the object, closing the file. */
	if(ttf_delete(font) == -1)
		{
		fprintf(stderr, "%s: ttf_delete() failed, %s\n", myname, ttf_strerror(ttf_errno(font)));
		gu_free(psname);
		return NULL;
		}

	p = font_info_new();
	p->font_psname = psname;
	p->font_type = gu_strdup("ttf");

	return p;
	} /* end of study_truetype_font() */

/*
** This function is used type parse lines from PostScript Type 1 fonts.
*/
static gu_boolean type1_line_process(struct FONT_INFO *fi, char *line)
	{
	if(!fi->font_family && strncmp(line, "/FamilyName (", 13) == 0)
		{
		fi->font_family = gu_strndup(line + 13, strcspn(line, ")"));
		return TRUE;
		}
	if(!fi->font_weight && strncmp(line, "/Weight (", 9) == 0)
		{
		fi->font_weight = gu_strndup(line + 9, strcspn(line, ")"));
		return TRUE;
		}
	if(!fi->font_slant && strncmp(line, "/ItalicAngle ", 13) == 0)
		{
		fi->font_slant = gu_strdup(atoi(line + 13) ? "o" : "r");
		return TRUE;
		}
	if(!fi->font_psname && strncmp(line, "/FontName /", 11) == 0)
		{
		fi->font_psname = gu_strndup(line + 11, strcspn(line + 11, " "));
		return TRUE;
		}
	if(!fi->font_encoding && strncmp(line, "/Encoding ", 10) == 0)
		{
		fi->font_encoding = gu_strndup(line + 10, strcspn(line + 11, " "));
		return TRUE;
		}
	if(!fi->font_type && strncmp(line, "/FontType ", 10) == 0)
		{
		fi->font_type = gu_strndup(line + 10, strcspn(line + 10, " "));
		return TRUE;
		}
	return FALSE;
	} /* end of type1_line_process() */

static struct FONT_INFO *study_pfa_font(const char filename[])
	{
	FILE *f;
	char *line = NULL;
	int line_available = 128;
	struct FONT_INFO *fi;

	if(!(f = fopen(filename, "r")))
		{
		fprintf(stderr, _("%s: can't open \"%s\", errno=%d (%s)\n"), myname, filename, errno, gu_strerror(errno));
		return NULL;
		}

	fi = font_info_new();

	while((line = gu_getline(line, &line_available, f)))
		{
		type1_line_process(fi, line);
		if(strcmp(line, "currentfile eexec") == 0)
			break;
		}

	fclose(f);

	if(fi->font_psname)
		{
		return fi;
		}
	else
		{
		font_info_delete(fi);
		return NULL;
		}
	} /* end of study_pfa_font() */

static struct FONT_INFO *study_pfb_font(const char filename[])
	{
	FILE *f;
	int c;
	int section_type;
	int x;
	int length;
	struct FONT_INFO *fi;

	if(!(f = fopen(filename, "r")))
		{
		fprintf(stderr, _("%s: can't open \"%s\", errno=%d (%s)\n"), myname, filename, errno, gu_strerror(errno));
		return NULL;
		}

	fi = font_info_new();

	while((c = fgetc(f)) != EOF)
		{
		if(c != 128)
			{
			fprintf(stderr, "\tFont is currupt.\n");
			break;
			}

		if((c = fgetc(f)) == EOF) break;
		section_type = c;

		length = 0;
		for(x=0; x<4 && (c = fgetc(f)) != EOF; x++)
			length += ((1 << (8 * x)) * c);
		if(c == EOF) break;

		/* printf("section_type=%d, length=%d\n", section_type, length); */

		if(section_type == 3) break;	/* end section */

		if(section_type == 1)
			{
			char line[256];
			while(length > 0)
				{
				for(x=0; length > 0 && x < (sizeof(line)-1); )
					{
					if((c = fgetc(f)) == EOF)
						{
						length = 0;
						break;
						}
					length--;
					if(c == '\r' || c == '\n') break;
					line[x++] = c;
					}
				line[x] = '\0';
				/* printf("\"%s\"\n", line); */
				type1_line_process(fi, line);
				}
			}
		else
			{
			fseek(f, length, SEEK_CUR);
			}

		break;
		}

	fclose(f);

	if(c == EOF)
		fprintf(stderr, "\tFont is corrupt or truncated.\n");


	if(fi->font_psname)
		{
		return fi;
		}
	else
		{
		font_info_delete(fi);
		return NULL;
		}
	} /* end of study_pfb_font() */

static struct FONT_INFO *study_afm_file(const char filename[])
	{
	struct FONT_INFO *p = NULL;

	return p;
	} /* end of study_afm_font() */

/*==========================================================================
** This routine is called for each file found in the font
** directories.
==========================================================================*/
static int do_file(FILE *indexfile, const char filename[])
	{
	unsigned char sample[16];
	int fd;
	ssize_t len;
	struct FONT_INFO *p;
    
	if((fd = open(filename, O_RDONLY)) == -1)
		{
		#if 0		/* appearently redundant code */
		if(errno == ENOENT)
			{
			printf(" -- Broken symbolic link");
			return EXIT_OK;
			}
		#endif
		fprintf(stderr, "\tCan't open \"%s\", errno=%d (%s)\n", filename, errno, gu_strerror(errno));
		return EXIT_INTERNAL;
		}

	/* Read a block from the begining of the file, which we
	   will examine for a magic number. */
	len = read(fd, sample, sizeof(sample));

	/* That was all we wanted. */
	close(fd);

	if(len < 0)
		{
		fprintf(stderr, "\tread() failed, errno=%d (%s)\n", errno, gu_strerror(errno));
		return EXIT_INTERNAL;
		}

	if(len > 4 && memcmp(sample, "\000\001\000\000", 4) == 0)
		{
		printf(" -- TrueType font file");
		p = study_truetype_font(filename);
		}
	else if(len > 14 && memcmp(sample, "%!PS-AdobeFont", 14) == 0)
		{
		printf(" -- PostScript font (PFA format)");
		p = study_pfa_font(filename);
		}
	else if(len > 1 && sample[0] == 128)
		{
		printf(" -- PostScript font (PFB format)");
		p = study_pfb_font(filename);
		}
	else if(len > 15 && memcmp(sample, "StartFontMetrics", 15) == 0)
		{
		printf(" -- Adobe Font Metrics file");
		p = study_afm_file(filename);
		}
	else
		{
		printf(" -- Probably not a scalable font");
		p = NULL;
		}

	if(p)
		{
		/* printf("psname = \"%s\"\n", p->font_psname); */
		fprintf(indexfile, "%s:%s:%s\n", p->font_psname, p->font_type ? p->font_type : "?", filename);
		font_info_delete(p);
		}
	#if 0
	else
		{
		fprintf(indexfile, "# %s\n", filename);
		}
	#endif

	return EXIT_OK;
	} /* end of do_file() */

/*============================================================================
** This routine is called for each font directory.
============================================================================*/
static int do_dir(FILE *indexfile, const char dirname[], int level)
	{
	DIR *dirobj;
	struct dirent *fileobj;
	char filename[MAX_PPR_PATH];
	struct stat statbuf;
	int retval = EXIT_OK;

	indent(level * 4);
	printf("%s\n", dirname);

	if(!(dirobj = opendir(dirname)))
		{
		if(errno == ENOENT)
			{
			printf("    -- Doesn't exist\n");
			return EXIT_OK;
			}
		fprintf(stderr, "%s: diropen(\"%s\") failed, errno=%d (%s)\n", myname, dirname, errno, gu_strerror(errno));
		return EXIT_INTERNAL;
		}

	while((fileobj = readdir(dirobj)))
		{
		/* Skip hidden files and the "." and ".." directories. */
		if(fileobj->d_name[0] == '.') continue;

		/* Build the full name of the file or directory. */
		ppr_fnamef(filename, "%s/%s", dirname, fileobj->d_name);

		if(stat(filename, &statbuf) == -1)
			{
			if(errno == ENOENT)
				{
				indent((level * 4) + 2);
				printf("%s", fileobj->d_name);
				printf(" -- Broken symbolic link\n");
				continue;
				}
			fprintf(stderr, "%s: stat() failed on \"%s\", errno=%d (%s)\n", myname, filename, errno, gu_strerror(errno));
			return EXIT_INTERNAL;
			}

		/* Search directories recursively. */
		if(S_ISDIR(statbuf.st_mode))
			{
			if((retval = do_dir(indexfile, filename, level + 1)) != EXIT_OK)
				break;
			continue;
			}

		/* Consider regular files to be possible fonts. */
		if(S_ISREG(statbuf.st_mode))
			{
			/* Skip obvious stuff. */
			if(strcmp(fileobj->d_name, "fonts.dir") == 0
				|| strcmp(fileobj->d_name, "fonts.scale") == 0
				|| gu_wildmat(fileobj->d_name, "*.pcf.gz")
				|| gu_wildmat(fileobj->d_name, "*.pcf")
				|| gu_wildmat(fileobj->d_name, "*.pfm")
				)
				continue;

			indent((level * 4) + 2);
			printf("%s", fileobj->d_name);
			retval = do_file(indexfile, filename);
			printf("\n");
			if(retval != EXIT_OK)
				break;
			continue;
			}
		}

	closedir(dirobj);

	return retval;
	} /* end of do_dir() */

/*============================================================================
** Main
============================================================================*/

int main(int argc, char *argv[])
	{
	FILE *indexfile;
	FILE *cf;
	struct GU_INI_ENTRY *section;
	int i;
	const char *dirname;
	int retval = EXIT_OK;

	/* Initialize international messages library. */
	#ifdef INTERNATIONAL
	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
	#endif

	/* Parse the options. */
	if(argc > 1)
		{
		if(strcmp(argv[1], "--delete") == 0)
			{
			unlink(fontindex_db);
			return EXIT_OK;
			}
		else
			{
			fprintf(stderr, "%s: unknown option: %s\n", myname, argv[1]);
			return EXIT_SYNTAX;
			}
		}

	/* Start a retry loop. */
	while(TRUE)
		{
		/* open ppr.conf */
		if(!(cf = fopen(ppr_conf, "r")))
			{
			fprintf(stderr, _("%s: can't open \"%s\", errno=%d (%s)\n"), myname, ppr_conf, errno, gu_strerror(errno));
			return EXIT_INTERNAL;
			}

		/* Fetch the [fonts] section */
		section = gu_ini_section_load(cf, section_name);

		fclose(cf);

		if(!section)
			{
			fprintf(stderr, _("%s: warning: no [%s] section in \"%s\", copying from \"%s.sample\"\n"), myname, section_name, ppr_conf, ppr_conf);
			if(gu_ini_section_from_sample(ppr_conf, section_name) == -1)
				return EXIT_INTERNAL;
			continue;
			}

		break;
		}
		
	/* Create the output file */
	if(!(indexfile = fopen(fontindex_db, "w")))
		{
		fprintf(stderr, _("%s: can't create \"%s\", errno=%d (%s)\n"), myname, fontindex_db, errno, gu_strerror(errno));
		return EXIT_INTERNAL;
		}

	/* Warn */
	fprintf(indexfile, "# This file format is a temporary hack.  It will change.\n");

	/* iterate over the nameless values */
	for(i=0; section[i].name; i++)
		{
		if(section[i].name[0] == '\0')
			{
			dirname = gu_ini_value_index(&section[i], 0, "<MISSING VALUE>");
			if((retval = do_dir(indexfile, dirname, 0)) != EXIT_OK)
				break;
			}
		}

	/* Free the in-memory representation of the [fonts] section. */
	gu_ini_section_free(section);

	/* Close the completed font index file. */
	fclose(indexfile);

	return retval;
	} /* end of main() */

/* end of file */

