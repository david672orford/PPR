/*
** mouse:~ppr/src/filter_dotmatrix/prop.c
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

/*
** This module contains proportional spacing tables for an Epson FX-850.
**
** This module is compiled twice, once without GENMETRICS defined to
** compile the tables and the function which reads them.  That code is used
** by the dot matrix printer emulator to advance the cursor properly.
** The second time it is compiled with GENMETRICS defined in order to
** compile a program which emmits these tables as PostScript /Metrics
** dictionaries.  These dictionaries are used to convert Courier to a
** proportionally space font.  One disadvantage of using /Metrics
** dictionaries is that some PostScript interpreters, such as Ghostscript
** before version 3.12, do not use them correctly.
*/

#include "filter_dotmatrix.h"

/*
** Proportional spacing table for Epson FX-850.
*/
int width_epson[224][2]={
		{12,12},				/* space */
		{5,10},					/* ! */
		{8,10},					/* " */
		{12,12},				/* # */
		{12,11},				/* $ */
		{12,12},				/* % */
		{12,12},				/* & */
		{5,5},					/* ' */
		{6,8},					/* ( */
		{6,8},					/* ) */
		{12,12},				/* * */
		{12,12},				/* + */
		{7,8},					/* , */
		{12,12},				/* - */
		{6,7},					/* . */
		{10,10},				/* / */
		{12,12},				/* 0 */
		{8,9},					/* 1 */
		{12,12},				/* 2 */
		{12,12},				/* 3 */
		{12,12},				/* 4 */
		{12,12},				/* 5 */
		{12,11},				/* 6 */
		{12,12},				/* 7 */
		{12,12},				/* 8 */
		{12,11},				/* 9 */
		{6,8},					/* : */
		{6,9},					/* ; */
		{10,10},				/* < */
		{12,11},				/* = */
		{10,9},					/* > */
		{12,11},				/* ? */
		{12,12},				/* @ */
		{12,12},				/* A */
		{12,12},				/* B */
		{12,12},				/* C */
		{12,12},				/* D */
		{12,12},				/* E */
		{12,12},				/* F */
		{12,12},				/* G */
		{12,12},				/* H */
		{8,10},					/* I */
		{11,12},				/* J */
		{12,12},				/* K */
		{12,10},				/* L */
		{12,12},				/* M */
		{12,12},				/* N */
		{12,12},				/* O */
		{12,12},				/* P */
		{12,12},				/* Q */
		{12,12},				/* R */
		{12,12},				/* S */
		{12,12},				/* T */
		{12,12},				/* U */
		{12,11},				/* V */
		{12,12},				/* W */
		{10,12},				/* X */
		{12,12},				/* Y */
		{10,12},				/* Z */
		{8,11},					/* [ */
		{10,7},					/* \ */
		{8,11},					/* ] */
		{12,10},				/* ^ */
		{12,12},				/* _ */
		{5,5},					/* ` */
		{12,11},				/* a */
		{11,11},				/* b */
		{11,11},				/* c */
		{12,12},				/* d */
		{12,11},				/* e */
		{10,12},				/* f */
		{11,11},				/* g */
		{11,11},				/* h */
		{8,9},					/* i */
		{9,10},					/* j */
		{10,11},				/* k */
		{8,9},					/* l */
		{12,11},				/* m */
		{11,10},				/* n */
		{12,11},				/* o */
		{12,11},				/* p */
		{11,11},				/* q */
		{11,10},				/* r */
		{12,11},				/* s */
		{11,10},				/* t */
		{12,11},				/* u */
		{12,10},				/* v */
		{12,12},				/* w */
		{10,12},				/* x */
		{12,11},				/* y */
		{10,12},				/* z */
		{9,10},					/* { */
		{5,9},					/* | */
		{9,10},					/* } */
		{12,12},				/* ~ */
		{12,12},				/* delete (7F) */
		{12,12},				/* C cedila */
		{11,12},				/* u dots */
		{12,11},				/* e accent */
		{12,12},				/* a hat */
		{12,11},				/* a dots */
		{12,11},				/* a other accent */
		{12,11},				/* a circle */
		{11,11},				/* c cedila */
		{12,12},				/* e hat */
		{12,11},				/* e dots */
		{12,11},				/* e accent */
		{8,10},					/* i dots */
		{10,11},				/* i hat */
		{8,8},					/* i accent */
		{12,12},				/* A dots */
		{12,12},				/* A circle */
		{12,12},				/* E accent */
		{12,12},				/* ae */
		{12,12},				/* AE */
		{10,12},				/* o hat */
		{10,11},				/* o dots */
		{10,11},				/* o accent */
		{11,11},				/* u hat */
		{11,11},				/* u accent */
		{12,11},				/* y dots */
		{12,12},				/* O dots */
		{12,12},				/* U dots */
		{11,11},				/* cents */
		{12,12},				/* pound sterling */
		{12,12},				/* yen */
		{12,12},				/* Pt */
		{11,12},				/* f */
		{12,11},				/* a accent */
		{8,10},					/* i accent */
		{10,12},				/* o accent */
		{11,11},				/* u accent */
		{11,12},				/* n tilde */
		{12,12},				/* N tilde */
		{12,11},				/* a underscore */
		{12,12},				/* o underscore */
		{12,11},				/* question */
		{12,12},				/* upper left corner */
		{12,12},				/* upper right corner */
		{12,12},				/* 1/2 */
		{12,12},				/* 1/4 */
		{5,10},					/* ! ? */
		{12,12},				/* << */
		{12,12},				/* >> */
		{12,12},				/* B0 */
		{12,12},				/* B1 */
		{12,12},				/* B2 */
		{12,12},				/* B3 */
		{12,12},				/* B4 */
		{12,12},				/* B5 */
		{12,12},				/* B6 */
		{12,12},				/* B7 */
		{12,12},				/* B8 */
		{12,12},				/* B9 */
		{12,12},				/* BA */
		{12,12},				/* BB */
		{12,12},				/* BC */
		{12,12},				/* BD */
		{12,12},				/* BE */
		{12,12},				/* BF */
		{12,12},				/* C0 */
		{12,12},				/* C1 */
		{12,12},				/* C2 */
		{12,12},				/* C3 */
		{12,12},				/* C4 */
		{12,12},				/* C5 */
		{12,12},				/* C6 */
		{12,12},				/* C7 */
		{12,12},				/* C8 */
		{12,12},				/* C9 */
		{12,12},				/* CA */
		{12,12},				/* CB */
		{12,12},				/* CC */
		{12,12},				/* CD */
		{12,12},				/* CE */
		{12,12},				/* CF */
		{12,12},				/* D0 */
		{12,12},				/* D1 */
		{12,12},				/* D2 */
		{12,12},				/* D3 */
		{12,12},				/* D4 */
		{12,12},				/* D5 */
		{12,12},				/* D6 */
		{12,12},				/* D7 */
		{12,12},				/* D8 */
		{12,12},				/* D9 */
		{12,12},				/* DA */
		{12,12},				/* DB */
		{12,12},				/* DC */
		{12,12},				/* DD */
		{12,12},				/* DE */
		{12,12},				/* DF */
		{12,12},				/* alpha */
		{11,11},				/* Beta */
		{10,12},				/* Lamda */
		{12,12},				/* pi */
		{10,12},				/* E ? */
		{11,12},				/* ? */
		{11,12},				/* micro */
		{12,12},				/* T ? */
		{10,12},				/* ? */
		{12,12},				/* ? */
		{12,12},				/* Omega */
		{12,11},				/* ? */
		{12,12},				/* infinity */
		{12,12},				/* empty set */
		{10,10},				/* E ? */
		{11,11},				/* ? */
		{11,11},				/* ? */
		{11,11},				/* +- */
		{11,11},				/* >_ */
		{11,11},				/* <_ */
		{11,11},				/* hook */
		{11,11},				/* crook */
		{11,11},				/* divide */
		{11,11},				/* aprox = */
		{11,11},				/* open circle */
		{11,11},				/* filled circle */
		{11,11},				/* dot */
		{11,11},				/* square root */
		{11,11},				/* en */
		{11,11},				/* squared */
		{11,11},				/* box */
		{12,12}					/* space */
		} ;

/*
** This routine is called from add_char() in linebuf.c
** It returns the 10 pitch width of the specified character
** in HORIZONTAL_UNITS.
*/
int width(int c, int italic)
	{
	if(c < 32)			/* for unlisted characters, */
		return 12;		/* return space width */

	if(italic)
		return width_epson[c-32][1] * (HORIZONTAL_UNITS/120);
	else
		return width_epson[c-32][0] * (HORIZONTAL_UNITS/120);
	} /* end of width */

/*=========================================================================
** These routines are compiled only when this is a standalone program for
** generating the Metrics dictionaries.
=========================================================================*/

#ifdef GENMETRICS

static int HORIZONTAL_UNITS=120;

static char CP437_names[256][16];				/* name from CP437 file */

/*
** Load the mapping from Code Page 437 to PostScript names.
*/
static void load_cp437(void)
	{
	FILE *f;
	int x;

	if(!(f = fopen("CP437", "r")))
		{
		fprintf(stderr, "Can't open CP437\n");
		exit(1);
		}

	for(x=0; x < 256; x++)
		{
		if(!fgets(CP437_names[x],16,f))			/* sloppy for simplicity */
			{
			fprintf(stderr, "Error reading CP437, premature EOF?\n");
			exit(1);
			}

		CP437_names[x][strcspn(CP437_names[x]," \t\n\r")] = '\0';
		}

	fclose(f);
	} /* end of load_cp437() */

/*
** Read the font's AFM file and use the bounding box information and the
** desired width from the above table to produce PostScript code to
** adjust the left side and width.
*/
static void genmetrics(char *infile, int italic)
	{
	FILE *in;
	char line[256];
	char *ptr;
	char name[256];				/* deliberatly sloppy */
	int found_N,found_B;
	int llx,lly,urx,ury;		/* bounding box */
	int i;
	int normal_width;
	int required_width;
	int lsb;
	int icount=0;				/* count of items on this line */

	if((in = fopen(infile,"r")) == (FILE*)NULL)
		{
		fprintf(stderr, "Can't open \"%s\", %d (%s)\n", infile, errno, gu_strerror(errno));
		exit(1);
		}

	while(strncmp(line,"StartCharMetrics",16))
		{
		if(fgets(line,sizeof(line),in) == (char*)NULL)
			{
			fprintf(stderr, "CharMetrics section not found in \"%s\".\n", infile);
			exit(1);
			}
		}

	while(fgets(line,sizeof(line),in) && line[0]=='C' )
		{
		found_N = found_B = FALSE;
		for(ptr=line; *ptr; ptr+=strcspn(ptr,";"), ptr+=strspn(ptr,";"), ptr+=strspn(ptr," \t") )
			{
			switch(*ptr)
				{
				case 'N':
					sscanf(ptr,"N %s", name);
					found_N = TRUE;
					break;
				case 'B':
					sscanf(ptr,"B %d %d %d %d", &llx, &lly, &urx, &ury);
					found_B = TRUE;
					break;
				default:
					break;
				}

			}

		/* make sure we got both name and bounding box */
		if( !found_N || !found_B )
			{
			fprintf(stderr, "Line without N and B:\n");
			fprintf(stderr, "%s", line);
			exit(1);
			}

		/* Find the CP437 encoding position of this character. */
		for(i=0; i < 256; i++)
			{
			if(strcmp(name, CP437_names[i]) == 0)		/* if match */
				{
				normal_width = urx - llx;
				required_width = (int)( (double)width(i,italic) * 600.0 / 12.0 + 0.5 );

				if(required_width==600 && strncmp(&name[4],"0000",4)==0)
					{					/* don't alter line draw if we can help it */
					lsb = llx;
					}
				else					/* most characters get new left */
					{					/* side bearing */
					/* lsb = ((normal_width - required_width) / 2); */
					lsb = llx - ((600 - required_width) / 2 );
					}

				#if 0
				printf("%% %d %s required_width=%d, normal_width=%d, lsb=%d, normal_lsb=%d\n",i,name,required_width,normal_width,lsb,llx);
				#endif

				/* If the computed metrics or other than normal, add to table. */
				if(required_width != 600 || lsb != llx)
					{
					gu_psprintf("/%s [%d %d] def", name, lsb, required_width);

					if(++icount == 2)			/* decide whether to */
						{						/* start a new line */
						icount=0;
						fputc('\n',stdout);
						}
					else
						{
						fputc(' ',stdout);
						}
					}

				/* Blank out the name to prevent re-use. */
				CP437_names[i][0] = '\0';
				break;
				} /* end if if match */
			} /* end of for loop to find name */

		} /* end of line loop */

	fclose(in);
	} /* end of genmetrics() */

int main(int argc, char *argv[])
	{
	if(argc != 4)
		{
		fprintf(stderr, "%s: invokation error\n", argv[0]);
		exit(1);
		}

	puts("%!PS-Adobe-3.0 Resource-Procset");
	gu_psprintf("%%%%Title: Auto Generated Epson %s metrics\n", argv[2]);
	puts("%%EndComments");

	load_cp437();
	puts("pprdotmatrix begin");
	gu_psprintf("/MetricsEpson_%s 256 dict def\n", argv[2]);
	gu_psprintf("MetricsEpson_%s begin\n", argv[2]);
	genmetrics(argv[1], atoi(argv[3]));
	gu_psprintf("end %% MetricsEpson_%s\n", argv[2]);

	puts("end % pprdotmatrix");
	puts("%%EOF");

	return 0;
	} /* end of main() */

#endif

/* end of file */
