/*
** mouse:~ppr/src/ipp/ipp_utils.h
** Copyright 1995--2000, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appears in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is"
** without express or implied warranty.
**
** Last modified 21 November 2000.
*/

int ipp_gsb(void *p);
int ipp_gss(void *p);
int ipp_gsi(void *p);
void ipp_ssb(void *p, int val);
void ipp_sss(void *p, int val);
void ipp_ssi(void *p, int val);
void debug(const char message[], ...);

/* end of file */
