%{
/*
** pod.l
** Last modified 23 June 1999.
*/

#include "pod_parser.h"
int debug = 1;
%}

%s MPOD
%x MEMBED

%%

^=pod		{ BEGIN MPOD;
		if(debug) printf("POD\n");
		return POD;
		}
^=head1		{ BEGIN MPOD;
		return HEAD1; }
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

<MPOD>^[ \t]+	{ if(debug) printf("VERBATIM\n");
		return VERBATIM; }

<MPOD>[ \t]+	|
<MPOD>[ \t]+\n	|
\n[ \t]+	|
\n		{ if(debug) printf("SPACE\n");
		return SPACE; }

[^ \t\n]+	{ if(debug) printf("WORD: \"%s\"\n", yytext);
		yylval.str = yytext;
		return WORD; }

[ \t]*\n[ \t]*\n[ \t\n]* {
		if(debug) printf("ENDPARAGRAPH\n");
		return ENDPARAGRAPH; }

[ \t]*\n/=	{ if(debug) printf("ENDPARAGRAPH (malformed)\n");
		return ENDPARAGRAPH; }

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

