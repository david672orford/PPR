%{
/*
** mouse:~ppr/src/filters_typeset/pod/pod.l
** Last modified 17 January 2002.
*/

#include "pod_parser.h"
int debug = 0;
%}

%s MPOD
%x MEMBED

%%

^=pod		{
		BEGIN MPOD;
		if(debug) printf("POD\n");
		return POD;
		}

^=head1		{
		BEGIN MPOD;
		return HEAD1;
		}

<MPOD>^=head2	{ return HEAD2; }
<MPOD>^=over	{ return OVER; }
<MPOD>^=back	{ return BACK; }
<MPOD>^=for	{ BEGIN MEMBED;
		return QFOR; }
<MPOD>^=begin	{ BEGIN MEMBED;
		return QBEGIN; }
<MEMBED>^=end	{ return QEND; }
<MEMBED>.	{ }
^=cut		{ BEGIN INITIAL; }

<MPOD>I<	{ return IOPEN; }
<MPOD>B<	{ return BOPEN; }
<MPOD>S<	{ return SOPEN; }
<MPOD>C<	{ return COPEN; }
<MPOD>L<	{ return LOPEN; }
<MPOD>F<	{ return FOPEN; }
<MPOD>X<	{ return XOPEN; }
<MPOD>X<lt>	{ return '<'; }
<MPOD>X<gt>	{ return '>'; }
<MPOD>Z<>	{ return ZEROWIDTH; }
<MPOD>E<	{ return EOPEN; }
<MPOD>>		{ return CLOSE; }

	/* Pod lines with leading whitespace are verbatim. */
<MPOD>^[ \t]+	{
		if(debug) printf("VERBATIM\n");
		return VERBATIM;
		}

	/* What does all this mean? */
<MPOD>[ \t]+	|
<MPOD>[ \t]+\n	|
\n[ \t]+	|
\n		{
		if(debug) printf("SPACE\n");
		return SPACE;
		}

	/* Non-space sequences that we haven't caught above are words. */
[^ \t\n]+	{
		if(debug) printf("WORD: \"%s\"\n", yytext);
		yylval.str = yytext;
		return WORD;
		}

	/* One or more blank lines end a paragraph. */
[ \t]*\n[ \t]*\n[ \t\n]* {
		if(debug) printf("ENDPARAGRAPH\n");
		return ENDPARAGRAPH;
		}

	/* If the next line begins with a POD command, that is the end of
	    the paragraph.  (Though there should have been a blank line.) */
[ \t]*\n/=	{
		if(debug) printf("ENDPARAGRAPH (malformed)\n");
		return ENDPARAGRAPH;
		}

	/* Nothing should get this far. */
.|\n		{ printf("ERROR!\n"); }

%%

void yyerror(const char *msg)
    {
    printf("error: %s (%s)\n", msg, yytext);
    }

int yywrap(void)
    {
    return 1;
    }

