/*
** mouse:~ppr/src/procsets/squeeze.c
** Copyright 1988 by Radical Eye Software.
**
** This utility program reduces the size of a PostScript code file by removing
** unnecessary white space and comments.  It comes from DVIPS.
**
** This code was ANSIfied and modified by David Chappell so that it no longer
** squeezes out DSC comments.  It was also changed so that is will not put a
** space before names (which change slightly increases the compression ratio)
** and so that it will not produce warnings when compiled with gcc -Wall
**
** Last modified 5 April 2003.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define LINELENGTH (72)
#define BUFLENGTH (1000)

#undef putchar
#define putchar(a) (void)putc(a, out);

FILE *in, *out;
static int linepos = 0;
static int lastspecial = 1;
char buf[BUFLENGTH];

/*
** This next routine writes out a `special' character.  In this case,
** we simply put it out, since any special character terminates the
** preceding token.
*/
static void specialout(char c)
	{
	if (linepos + 1 > LINELENGTH)
		{
		putchar('\n');
		linepos = 0;
		}
	putchar(c);
	linepos++;
	lastspecial = 1;
	}

/*
** Write a PostScript ASCII or Hex string
*/
static void strout(char *s)
	{
	if (linepos + strlen(s) > LINELENGTH)		/* If line would be to long with the addition */
		{										/* of this string, start a new line. */
		putchar('\n');
		linepos = 0;
		}
	linepos += strlen(s);						/* Keep track of line length. */
	while (*s != 0)								/* Emmit the string. */
		putchar(*s++);
	lastspecial = 1;							/* Closing ) or > is a special */
	}

/*
** Emmit a PostScript command word.
*/
static void cmdout(char *s)
	{
	int l;

	l = strlen(s);

	if (linepos + l + 1 > LINELENGTH)	/* If emmiting this command could */
		{								/* cause this line to become too long, */
		putchar('\n');					/* break off the line right here. */
		linepos = 0;
		lastspecial = 1;
		}

	if (! lastspecial && *s != '/')		/* If the last character was not a special, */
		{								/* insert a seperator space to before this command. */
		putchar(' ');
		linepos++;
		}

	while (*s != 0)						/* Write the command. */
		putchar(*s++);

	linepos += l;
	lastspecial = 0;
	}

/*
** Main function and main loop.
*/
int main(int argc, char *argv[])
	{
	int c;
	char *b;
	char seeking;

	if (argc > 3 || (in=(argc < 2 ? stdin : fopen(argv[1], "r")))==NULL ||
					(out=(argc < 3 ? stdout : fopen(argv[2], "w")))==NULL)
		{
		fprintf(stderr, "Usage:	 squeeze [infile [outfile]]\n") ;
		exit(1);
		}

	while(1)
		{
		if( (c = getc(in)) == EOF )
			break;

		if (c=='%')						/* If it is a comment, */
			{
			c = getc(in);				/* get second character */

			if( c=='%' || c=='!' )		/* test for %% or %! */
				{
				if(linepos != 0)		/* If in the middle of a line, */
					{					/* start a new one. */
					putchar('\n');
					linepos = 0;
					}

				putchar('%'); putchar('%');

				while( (c=getc(in)) != EOF && c != '\n' )
					putchar(c);

				putchar('\n');
				}
			else						/* Eat up the comment */
				{						/* Exact form of this loop is critical! */
				while( c != EOF && c != '\n' )
					c = getc(in);
				}

			continue;
			}

		if(c <= ' ')					/* delete all spaces, we will put back those we need */
			continue ;

		switch(c)
			{
			case '{':					/* scanner break characters */
			case '}':
			case '[':
			case ']':
				specialout(c);
				break;

			case '<':					/* start of ASCII or Hex string */
			case '(':
				if (c=='(')
					seeking = ')';
				else
					seeking = '>';

				b = buf;				/* set b to start of buffer */
				*b++ = c;						/* store character in buffer */

				do {
					if( (c = getc(in)) == EOF )
						{
						fprintf(stderr, "EOF in string\n");
						exit(1);
						}
					if (b > buf + BUFLENGTH-2)
						{
						fprintf(stderr, "Overran buffer seeking %c", seeking);
						exit(1);
						}
					*b++ = c ;
					if (c=='\\')
						*b++ = getc(in);
					} while (c != seeking);

				*b++ = '\0';
				strout(buf);					/* emmit the whole string */
				break;

			default:							/* normal stuff */
				b = buf ;
				while ((c>='A'&&c<='Z')||(c>='a'&&c<='z')||
						(c>='0'&&c<='9')||(c=='/')||(c=='@')||
						(c=='!')||(c=='"')||(c=='&')||(c=='*')||(c==':')||
						(c==',')||(c==';')||(c=='?')||(c=='^')||(c=='~')||
						(c=='-')||(c=='.')||(c=='#')||(c=='|')||(c=='_')||
						(c=='=')||(c=='$')||(c=='+'))
					{
					*b++ = c;
					c = getc(in);
					}
				ungetc(c, in);
				if (b == buf)					/* if we didn't get anything, */
					{
					fprintf(stderr, "Oops!  Missed a case: %c.\n", c);
					exit(1) ;
					}
				*b++ = '\0';
				cmdout(buf);
				break;
			}
		}

	if(linepos != 0)
		putchar('\n') ;

	return 0;
	} /* end of main() */

/* end of file */
