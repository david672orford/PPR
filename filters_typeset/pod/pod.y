%{
/*
** mouse:~ppr/src/filters_typeset/pod/pod.y
** Last modified 17 January 2002.
*/
%}

%union {
	char *str;
	}

%token POD
%token HEAD1
%token HEAD2
%token OVER
%token BACK
%token QFOR
%token QBEGIN
%token QEND
%token CUT

%token IOPEN
%token BOPEN
%token SOPEN
%token COPEN
%token LOPEN
%token FOPEN
%token XOPEN
%token ZEROWIDTH
%token EOPEN
%token CLOSE

%token <str> WORD
%token SPACE
%token ENDPARAGRAPH
%token VERBATIM

%%

	/* A document consists of a paragraph or a valid 
		document with one more paragraph. */
doc: paragraph
	| doc paragraph

	/* paragraph: sub_paragraph ENDPARAGRAPH */

paragraph: command			{ printf("<p>\n"); }
	| WORD 				{ printf("%s", $1); }
	| paragraph SPACE WORD		{ printf(" %s", $3); }
	| paragraph ENDPARAGRAPH	{ printf("</p>\n"); }

command:	HEAD1
	|	HEAD2

%%

int main(int argc, char *argv[])
    {
    yyparse();
    return 0;
    }



