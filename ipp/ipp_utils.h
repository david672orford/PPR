/*
** mouse:~ppr/src/ipp/ipp_utils.h
** Copyright 1995--2002, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appears in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is"
** without express or implied warranty.
**
** Last modified 12 August 2002.
*/

struct IPP
    {
    int bytes_left;
    unsigned char readbuf[512];
    int readbuf_i;
    int readbuf_remaining;
    unsigned char writebuf[512];
    int writebuf_i;
    int writebuf_remaining;
    };

struct IPP *ipp_new(int content_length);
void ipp_end(struct IPP *p);
int ipp_get_sb(struct IPP *p);
int ipp_get_ss(struct IPP *p);
int ipp_get_si(struct IPP *p);
void ipp_put_sb(struct IPP *p, int val);
void ipp_put_ss(struct IPP *p, int val);
void ipp_put_si(struct IPP *p, int val);
void debug(const char message[], ...);

/* end of file */
