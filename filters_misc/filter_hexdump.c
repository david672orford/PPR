/*
** mouse:~ppr/src/misc_filters/filter_hexdump.c
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
** Last modified 1 November 2003.
*/

/*
** Hex Dump Filter for the PPR spooling system.
**
** Usage: filter_hexdump _options_ _printer_name_ _file_type_description_
**
** The only option currently supported is "maxpages=".  The default is 1.
**
** The third parameter is a brief phrase describing the file type.  This
** parameter position is normally used for the job title.
*/

#include "before_system.h"
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#ifdef INTERNATIONAL
#include <locale.h>
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"

const char myname[] = "filter_hexdump";

static void emmit_header(const char PageSize[], const double page_height, const double top_bottom_margin, double left_right_margin, const double pointsize, const double line_spacing)
	{
	puts("%!PS-Adobe-3.0");
	puts("%%DocumentData: Clean7Bit");
	puts("%%Title: Hex Dump of Bad Job");
	puts("%%Creator: PPR hex dump filter");
	puts("%%Pages: (atend)");
	puts("%%DocumentNeededResources: font Courier Helvetica");
	puts("%%ProofMode: TrustMe");
	puts("%%EndComments\n");

	puts("%%BeginProlog");
	printf("/PH %.2f def\n", page_height);
	printf("/TBM %.2f def\n", top_bottom_margin);
	printf("/LRM %.2f def\n", left_right_margin);
	printf("/LS %.2f def\n", line_spacing);
	printf("/PT %.2f def\n", pointsize);
	puts("/bp{save LRM PH TBM sub moveto}def");
	puts("/nl{currentpoint exch pop LS sub LRM exch moveto}def");
	puts("/p{show nl}def");
	puts("/ep{showpage restore}def");
	puts("%%EndProlog\n");

	puts("%%BeginSetup");
	printf("%%%%IncludeFeature: *PageSize %s\n", PageSize);
	puts("%%IncludeResource: font Courier");
	puts("%%IncludeResource: font Helvetica");
	puts("/C /Courier findfont 10 scalefont def");
	puts("/H /Helvetica findfont 10 scalefont def");
	puts("%%EndSetup\n");
	} /* end of emmit_header() */

static void begin_page(int page)
	{
	printf("%%%%Page: %d %d\n", page, page);
	puts("bp");
	}

static void end_page(void)
	{
	puts("ep\n");
	}

static void emmit_trailer(int pages)
	{
	puts("%%Trailer");
	printf("%%%%Pages: %d\n", pages);
	puts("%%EOF");
	}

static void hexline(unsigned char *segment, int len)
	{
	int x;
	int c;

	/* First, print as characters */
	fputc('(', stdout);

	for(x=0; x < len; x++)		/* do each character in line */
		{
		c = segment[x];

		if(!isprint(c))		/* if unprintable, use dot */
			{
			fputc('.', stdout);
			}
		else			/* if printable, print */
			{
			switch(c)
				{
				case '(':
				case ')':
				case 0x5C:
					printf("\\%c",c);
					break;
				default:
					fputc(c,stdout);
					break;
				}
			}
		}

	/* Padd out to 16 characters */
	for( ; x < 16; x++)
		{
		fputc(' ',stdout);
		}

	/* print in hexadecimal */
	for(x=0; x < len; x++)
		{
		printf(" %2.2X",segment[x]);
		}

	puts(")p");
	} /* end of hexline() */

int main(int argc, char *argv[])
	{
	const double inch = 72.0;
	const double pointsize = 10.0;
	const double line_spacing = 12.0;
	const double top_bottom_margin = 0.5 * inch;
	const double left_right_margin = 0.5 * inch;

	const char *PageSize;
	double phys_pu_width, phys_pu_height;
	unsigned char buffer[16];
	int len;
	int line;
	int page;
	int opt_maxpages = 1;
	int lines_per_page;

        /* Initialize international messages library. */
        #ifdef INTERNATIONAL
        setlocale(LC_ALL, "");
        bindtextdomain(PACKAGE, LOCALEDIR);
        textdomain(PACKAGE);
        #endif

        /* Read the default pagesize from the PPR config file. */
        {
        const char file[] = PPR_CONF;
        const char section[] = "internationalization";
        const char key[] = "defaultmedium";
        const char *error_message;

        error_message = gu_ini_scan_list(file, section, key,
            GU_INI_TYPE_NONEMPTY_STRING, &PageSize,
            GU_INI_TYPE_POSITIVE_DOUBLE, &phys_pu_width,
            GU_INI_TYPE_POSITIVE_DOUBLE, &phys_pu_height,
            GU_INI_TYPE_END);

        if(error_message)
            {
            fprintf(stderr, _("%s: %s\n"
                            "\twhile attempting to read \"%s\"\n"
                            "\t\t[%s]\n"
                            "\t\t%s =\n"),
                    myname, gettext(error_message), file, section, key);
            exit(1);
            }
        }

	/* Parse the filter options. */
        if(argc >= 2)
            {
            struct OPTIONS_STATE o;
            char name[32], value[64];
            int rval;

            /* Handle all the name=value pairs. */
            options_start(argv[1], &o);
            while((rval = options_get_one(&o, name,sizeof(name),value,sizeof(value))) == 1)
                {
		if(strcmp(name, "maxpages") == 0)
		    opt_maxpages = atoi(value);
                }

            /*
            ** If options_get_one() detected an error, print it now.
            */
            if(rval == -1)
                {
                filter_options_error(1, &o, gettext(o.error));
                exit(1);
                }
            } /* end of if there are options */

	/* Figure out how many lines we can get an a page. */
	lines_per_page = (int)((phys_pu_height - top_bottom_margin - top_bottom_margin) / line_spacing);

	/* Create all of the PostScript up to the start of the first page. */
	emmit_header(PageSize, phys_pu_height, top_bottom_margin, left_right_margin, pointsize, line_spacing);

	/* Start the first page. */
	page = 1;
	line = 0;
        begin_page(page);

	/* If we know the file type, description, print it in a pretty message. */
        if(argc >= 4)
            {
            puts("H setfont");
            printf("(Can't print %s.  Here is a sample)p\n", argv[3]);
            puts("(of the contents of your job:)p");
            puts("nl");
            line += 3;
            }

	puts("C setfont");

	/* Print pages. */
	while((len = fread(buffer, sizeof(unsigned char), sizeof(buffer), stdin)))
	    {
	    if(++line > lines_per_page)
		{
		if(page == opt_maxpages) break;
		end_page();
		begin_page(++page);
		puts("C setfont");
		line = 0;
		}
	    hexline(buffer,len);
            }

	/* Close the last page. */
        end_page();

	/* Close the PostScript. */
	emmit_trailer(page);

	/* Eat up the rest of the input to avoid broken pipe messages. */
	/* while(freed(buffer, sizeof(unsigned char), sizeof(buffer), stdin)) ; */

	return 0;
	} /* end of main() */

/* end of file */

