%{
/*
** pod.y
** Last modified 23 June 1999.
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

doc: paragraph
	| doc paragraph

	/* paragraph: sub_paragraph ENDPARAGRAPH */

paragraph: command			{ printf("COMMAND\n"); }
	| WORD 				{ printf("P: \"%s\"\n", $1); }
	| paragraph SPACE WORD	{ printf("P: \" %s\"\n", $3); }
	| paragraph ENDPARAGRAPH

command:	HEAD1
	|	HEAD2

%%

int main(int argc, char *argv[])
    {
    yyparse();
    return 0;
    }



