/*
** mouse:~ppr/src/misc/ppr-testpage.c
** Copyright 1995--2004, Trinity College Computing Center.
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
** Last modified 15 April 2004.
*/

#include "before_system.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#ifdef INTERNATIONAL
#include <locale.h>
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "util_exits.h"
#include "version.h"

const char myname[] = "ppr-testpage";

void error(const char *message, ... )
	{
	va_list va;
	fprintf(stderr, "%s: ", myname);
	va_start(va,message);
	vfprintf(stderr,message,va);
	va_end(va);
	fputc('\n', stderr);
	} /* end of error() */

/*========================================================================
**
========================================================================*/

static void do_header(void)
	{
	gu_psprintf(
		"%%!PS-Adobe-3.0\n"
		"%%%%Title: PPR Test Page\n"
		"%%%%Pages: 1\n"
		"%%%%DocumentNeededResources: font Helvetica\n"
		"%%%%EndComments\n"
		);
	}

static void do_prolog(void)
	{
	gu_psprintf(
		"%%%%BeginProlog\n"
		"%%%%EndProlog\n"
		);
	}

static void do_setup(const char page_size[])
	{
	gu_psprintf(
		"%%%%BeginSetup\n"
		"%%%%IncludeResource: font Helvetica\n"
		"/Helvetica findfont 10 scalefont setfont\n"
		"%%%%IncludeFeature: *PageSize %s\n"
		"10 dict begin\n"
		"%%%%EndSetup\n"
		, page_size);
	}

static void do_startpage(int num)
	{
	gu_psprintf(
		"%%%%Page: %d %d\n"
		"save\n"
		, num, num);
	}

static void do_endpage(void)
	{
	gu_psprintf(
		"%% End of page\n"
		"restore\n"
		"showpage\n"
		);
	}

static void do_trailer(void)
	{
	gu_psprintf(
		"%%%%Trailer\n"
		"end\n"
		"%%%%EOF\n"
		);
	}

/*========================================================================
** Drawing routines
========================================================================*/

static void do_border(double pw, double ph, double border_width, const char setdash[])
	{
	gu_psprintf("%% border\n");
	gu_psprintf("gsave\n");
	gu_psprintf("0 setlinewidth\n");
	if(setdash)
		gu_psprintf("%s setdash\n", setdash);
	gu_psprintf("newpath %f dup moveto 0 %f rlineto %f 0 rlineto 0 -%f rlineto\n",
		border_width,
		(ph - (border_width*2)),
		(pw - (border_width*2)),
		(ph - (border_width*2)) );
	gu_psprintf("closepath stroke\n");
	gu_psprintf("grestore\n");
	gu_psprintf("\n");
	}

/* This structure describes an EPS bounding box. */
struct BBOX {
	int llx;
	int lly;
	int urx;
	int ury;
	};

/*
** This routine opens an EPS file and extracts its bounding box.
*/
static gu_boolean eps_get_bbox(const char filename[], struct BBOX *bbox)
	{
	FILE *eps;
	char *line = NULL;
	int line_len = 80;
	gu_boolean found = FALSE;

	if(!(eps = fopen(filename, "r")))
		{
		fprintf(stderr, "%s: can't open \"%s\", errno=%d (%s)\n", myname, filename, errno, gu_strerror(errno));
		return FALSE;
		}

	while((line = gu_getline(line, &line_len, eps)))
		{
		if(gu_sscanf(line, "%%%%BoundingBox: %d %d %d %d", &bbox->llx, &bbox->lly, &bbox->urx, &bbox->ury) == 4)
			{
			found = TRUE;
			break;
			}
		}

	if(line)
		gu_free(line);

	fclose(eps);

	if(!found)
		{
		fprintf(stderr, "%s: no bounding box information in \"%s\"\n", myname, filename);
		return FALSE;
		}

	return TRUE;
	}

/*
** Include an EPS file (with a bounding box determined by eps_get_bbox()).
** The EPS file is wrapped with the code suggested in RBII p. 726.  However,
** the Red Book ommits mention of the need to clip.
**
** The parameters are the name of the file, its bounding box, the X and Y
** coordinates of the lower left corner of the rectangle it should appear
** in and the factor by which to scale its size.
*/
static gu_boolean eps_insert(const char filename[], struct BBOX *bbox, double x, double y, double scale)
	{
	FILE *eps;
	char *line = NULL;
	int line_len = 80;

	if(!(eps = fopen(filename, "r")))
		{
		fprintf(stderr, "%s: can't open \"%s\", errno=%d (%s)\n", myname, filename, errno, gu_strerror(errno));
		return FALSE;
		}

	/* Here is the Red Book code to save the current state. */
	gu_psprintf(
		"%% Start of EPS setup\n"
		"/b4_Inc_state save def\n"
		"/dict_count countdictstack def\n"
		"/op_count count 1 sub def\n"
		"userdict begin\n"
		"/showpage {} def\n"
		);

	/* Establish a coordinate system with its origin at the lower left corner
	   of the box we want the EPS file to appear in. */
	gu_psprintf("%f %f translate\n", x, y);

	/* Scale the EPS file. */
	gu_psprintf("%f dup scale\n", scale);

	/* Make an adjustment to account for EPS files which don't have (0,0) as their origin. */
	gu_psprintf("%d neg %d neg translate\n", bbox->llx, bbox->lly);

	/* Clip to the EPS file's claimed bounding box.  If we don't do this,
	   the Ghostscript tiger will paint everthing gray. */
	gu_psprintf("newpath %d %d moveto %d %d lineto %d %d lineto %d %d lineto closepath clip\n",
		bbox->llx, bbox->lly,
		bbox->llx, bbox->ury,
		bbox->urx, bbox->ury,
		bbox->urx, bbox->lly
		);

	gu_psprintf("%% End of EPS setup\n\n");

	gu_psprintf("%%%%BeginDocument: %s\n", filename);

	while((line = gu_getline(line, &line_len, eps)))
		{
		gu_psprintf("%s\n", line);
		}

	gu_psprintf(
		"%%%%EndDocument\n"
		"\n"
		"%% Start of EPS cleanup\n"
		"count op_count sub {pop} repeat\n"
		"countdictstack dict_count sub {end} repeat\n"
		"b4_Inc_state restore\n"
		"%% End of EPS cleanup\n"
		);

	fclose(eps);
	return TRUE;
	}

/*========================================================================
** Test pattern routines
========================================================================*/

static void test_graybar(int x, int y, int width, int height, const char setproc[], const char towhite[])
	{
	gu_psprintf(
		"%% grayscale bar\n"
		"save\n"
		"/x %d def\n"
		"/y %d def\n"
		"/width_each %d def\n"
		"/height %d def\n"
		"/temp 10 string def\n"
		"/setproc { %s } def\n"
		"/towhite { %s } def\n",
		x,
		y,
		width / 11,
		height,
		setproc,
		towhite);

	gu_psprintf(
		"0 setlinewidth\n"
		"0 1 10 {\n"
		"	/i exch def\n"
		"	newpath\n"
		"	x i width_each mul add y moveto		%% bottom left\n"
		"		0 height rlineto				%% to top left\n"
		"		width_each 0 rlineto			%% to top right\n"
		"		0 height neg rlineto			%% to bottom right\n"
		"		closepath						%% back to bottom left\n"
		"		gsave\n"
		"		  i 10 div setproc\n"
		"		  fill\n"
		"		  grestore\n"
		"		stroke\n"
		"	i 10 mul temp cvs length			%% multiply by 10 to get percentage\n"
		"	  temp exch\n"
		"	  2 copy (%%) putinterval			%% add percent sign\n"
		"	  0 exch 1 add getinterval			%% reduce to string\n"
		"	  /str exch def\n"
		"	  x									%% base x\n"
		"		i width_each mul add			%% plus offset to start of block\n"
		"		width_each 2 div add			%% plus half a block\n"
		"		str stringwidth pop 2 div sub	%% plus half the string width\n"
		"	  y 5 add moveto\n"
		"		gsave\n"
		"		i towhite { 1.0 setgray } if\n"
		"		str show\n"
		"		grestore\n"
		"	} for\n"
		"restore\n"
		);

	}

static void test_spoked_wheel(int x, int y, int radius)
	{
	gu_psprintf(
		"gsave\n"
		"0 setlinewidth\n"
		"0 setgray\n"
		"%d %d translate\n",
		x + radius, y - radius
		);

	gu_psprintf(
		"0 25 %d\n"
		"  {\n"
		"  newpath 0 0 3 -1 roll 0 360 arc closepath stroke\n"
		"  } for\n",
		radius
		);

	gu_psprintf(
		"0 1 359\n"
		"  {\n"
		"  pop\n"
		"  newpath\n"
		"  0 0 moveto\n"
		"  0 %d lineto\n"
		"  stroke\n"
		"  1 rotate\n"
		"  } for\n",
		radius
		);

	gu_psprintf("grestore\n");
	}

/*========================================================================
** Main and support routines
========================================================================*/

/*
** Command line options:
*/
static const char *option_chars = "";
static const struct gu_getopt_opt option_words[] =
		{
		{"help", 1000, FALSE},
		{"version", 1001, FALSE},
		{"pagesize", 1002, TRUE},
		{"eps-file", 1003, TRUE},
		{"eps-scale", 1004, TRUE},
		{"test-grayscale", 1005, FALSE},
		{"test-rgb", 1006, FALSE},
		{"test-cmyk", 1007, FALSE},
		{"test-spokes", 1008, FALSE},
		{(char*)NULL, 0, FALSE}
		} ;

/*
** Print help.
*/
static void help_switches(FILE *outfile)
	{
	fputs(_("Valid switches:\n"), outfile);

	fputs(_(	"\t--pagesize=<pagesize>\n"), outfile);
	fputs(_(	"\t--eps-file=<filename>\n"), outfile);
	fputs(_(	"\t--eps-scale=<float>\n"), outfile);
	fputs(_(	"\t--test-grayscale\n"), outfile);
	fputs(_(	"\t--test-rgb\n"), outfile);
	fputs(_(	"\t--test-cmyk\n"), outfile);
	fputs(_(	"\t--test-spokes\n"), outfile);

	fputs(_(	"\t--version\n"
				"\t--help\n"), outfile);
	}

int main(int argc, char *argv[])
	{
	const char *PageSize = "Letter";
	const char *eps_file = SHAREDIR"/www/images/pprlogo2.eps";
	int eps_margin = 40;
	double eps_scale = 0.40;
	double ph, pw;				/* page width and height */
	int x, y;					/* work variables */
	gu_boolean test_grayscale = FALSE;
	gu_boolean test_rgb = FALSE;
	gu_boolean test_cmyk = FALSE;
	gu_boolean test_spokes = FALSE;

	/* Initialize international messages library. */
	#ifdef INTERNATIONAL
	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
	#endif

	pw = 8.5 * 72.0;
	ph = 11.0 * 72.0;

	/* Parse the options. */
	{
	struct gu_getopt_state getopt_state;
	int optchar;
	gu_getopt_init(&getopt_state, argc, argv, option_chars, option_words);
	while((optchar = ppr_getopt(&getopt_state)) != -1)
		{
		switch(optchar)
			{
			case 1000:					/* --help */
				help_switches(stdout);
				exit(EXIT_OK);

			case 1001:					/* --version */
				puts(VERSION);
				puts(COPYRIGHT);
				puts(AUTHOR);
				exit(EXIT_OK);

			case 1002:					/* --pagesize= */
				{
				char *p = NULL;
				if(pagesize(getopt_state.optarg, &p, &pw, &ph, NULL) == -1)
					{
					fprintf(stderr, "%s: unknown --pagesize %s\n", myname, getopt_state.optarg);
					exit(EXIT_NOTFOUND);
					}
				if(p)
					PageSize = p;
				else
					PageSize = getopt_state.optarg;
				}
				break;

			case 1003:					/* --eps-file= */
				eps_file = getopt_state.optarg;
				break;

			case 1004:					/* --eps-scale= */
				eps_scale = strtod(getopt_state.optarg, NULL);
				break;

			case 1005:
				test_grayscale = TRUE;
				break;

			case 1006:
				test_rgb = TRUE;
				break;

			case 1007:
				test_cmyk = TRUE;
				break;

			case 1008:
				test_spokes = TRUE;
				break;

			default:					/* other getopt errors or missing case */
				gu_getopt_default(myname, optchar, &getopt_state, stderr);
				exit(EXIT_SYNTAX);
				break;
			}
		}
	}

	do_header();

	do_prolog();

	do_setup(PageSize);

	do_startpage(1);

	do_border(pw, ph, 36.0, NULL);				/* 1/2" */
	do_border(pw, ph, 31.5, "[1 1] 1");			/* 7/16" */
	do_border(pw, ph, 27.0, "[3 1] 2");			/* 3/8" */
	do_border(pw, ph, 22.5, "[1 1] 1");			/* 5/16" */
	do_border(pw, ph, 18.0, NULL);				/* 1/8" */
	do_border(pw, ph, 13.5, "[1 1] 1");			/* 3/16" */
	do_border(pw, ph, 09.0, "[3 1] 2");			/* 1/8" */
	do_border(pw, ph, 04.5, "[1 1] 1");			/* 1/16" */

	/* We place the EPS file at the upper right. */
	{
	struct BBOX bbox;
	if(!eps_get_bbox(eps_file, &bbox))
		return 10;
	x = pw - eps_margin - ((bbox.urx - bbox.llx) * eps_scale);
	y = ph - eps_margin - ((bbox.ury - bbox.lly) * eps_scale);
	eps_insert(eps_file, &bbox,
		x,
		y,
		eps_scale);
	}

	x = eps_margin;
	y -= 15;

	/* Grayscale */
	if(test_grayscale)
		{
		y -= 10;
		gu_psprintf("%d %d moveto (Grayscale) show\n", x, y);
		y -= 38;
		test_graybar(x, y, (pw - eps_margin - eps_margin), 36,
				"1 exch sub setgray", "6 gt");
		y -= 15;
		}

	/* RGB */
	if(test_rgb)
		{
		y -= 10;
		gu_psprintf("%d %d moveto (Red) show\n", x, y);
		y -= 38;
		test_graybar(x, y, (pw - eps_margin - eps_margin), 36,
				"0 0 setrgbcolor", "6 lt");

		y -= 10;
		gu_psprintf("%d %d moveto (Green) show\n", x, y);
		y -= 38;
		test_graybar(x, y, (pw - eps_margin - eps_margin), 36,
				"0 0 3 1 roll setrgbcolor", "6 lt");

		y -= 10;
		gu_psprintf("%d %d moveto (Blue) show\n", x, y);
		y -= 38;
		test_graybar(x, y, (pw - eps_margin - eps_margin), 36,
				"0 0 3 2 roll setrgbcolor", "pop true");

		y -= 15;
		}

	/* CMYK */
	if(test_cmyk)
		{
		y -= 10;
		gu_psprintf("%d %d moveto (Cyan) show\n", x, y);
		y -= 38;
		test_graybar(x, y, (pw - eps_margin - eps_margin), 36,
				"0 0 0 setcmykcolor", "pop false");

		y -= 10;
		gu_psprintf("%d %d moveto (Magenta) show\n", x, y);
		y -= 38;
		test_graybar(x, y, (pw - eps_margin - eps_margin), 36,
				"0 0 0 4 1 roll setcmykcolor", "pop false");

		y -= 10;
		gu_psprintf("%d %d moveto (Yellow) show\n", x, y);
		y -= 38;
		test_graybar(x, y, (pw - eps_margin - eps_margin), 36,
				"0 0 0 4 2 roll setcmykcolor", "pop false");

		y -= 10;
		gu_psprintf("%d %d moveto (Black) show\n", x, y);
		y -= 38;
		test_graybar(x, y, (pw - eps_margin - eps_margin), 36,
				"0 0 0 4 3 roll setcmykcolor", "6 gt");

		y -= 15;
		}

	/* spoked wheel */
	if(test_spokes)
		{
		/* y -= 10; */
		test_spoked_wheel(x, y, 125);
		y -= 250;
		}


	/* Print the product name. */
	x = eps_margin;
	y = eps_margin;
	gu_psprintf(
		"%d %d moveto\n"
		"statusdict begin\n"
		"  (Product: ) show product show\n"
		"  (	  Version: ) show version 10 string cvs show\n"
		"  (	  Revision: ) show revision 10 string cvs show\n"
		"end\n"
		, x, y);

	do_endpage();

	do_trailer();

	return 0;
	} /* end of main() */

/* end of file */
