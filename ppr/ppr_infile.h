/*
** mouse:~ppr/ppr/ppr_infile.h
** Copyright 1998, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is" without
** express or implied warranty.
**
** Last modified 7 October 1998.
*/

gu_boolean in_eof(void);
int infile_open(const char filename[]);
void in_getline(void);
int in_getc(void);
void in_ungetc(int c);
void infile_close(void);
int infile_force_type(const char type[]);
void minus_tee_help(FILE *outfile);
void infile_file_cleanup(void);

/* end of file */

