/* Stuff shared by pprdrv_ppd_parse.l
   and pprdrv_ppd.c */
extern FILE *yyin;		/* lex's input file */
int yylex(void);		/* lex's principle function */
void ppd_callback_add_font(char *fontname);
void ppd_callback_new_string(const char name[]);
void ppd_callback_string_line(char *string);
void ppd_callback_end_string(void);
void ppd_callback_order_dependency(const char text[]);
void ppd_callback_papersize_moveto(char *paper);
extern int papersizex;
extern int ppd_nest_level;
extern char *ppd_nest_fname[MAX_PPD_NEST];
void ppd_callback_rip(const char text[]);
void ppd_callback_cups_filter(const char text[]);
void ppd_callback_resolution(const char text[]);
