/*
** mouse:~ppr/src/ipp/ipp_utils.c
** Copyright 1995--2002, Trinity College Computing Center.
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
** Last modified 12 August 2002.
*/

#include "before_system.h"
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include "gu.h"
#include "global_defines.h"
#include "ipp_constants.h"
#include "ipp_except.h"
#include "ipp_utils.h"

struct IPP *ipp_new(int content_length)
    {
    struct IPP *p = gu_alloc(1, sizeof(struct IPP));

    p->bytes_left = content_length;

    p->readbuf_i = 0;
    p->readbuf_remaining = 0;

    p->writebuf_i = 0;
    p->writebuf_remaining = sizeof(p->writebuf);

    return p;
    }

static void ipp_readbuf_load(struct IPP *p)
    {
    if((p->readbuf_remaining = read(0, p->readbuf, p->bytes_left < sizeof(p->readbuf) ? sizeof(p->readbuf) : p->bytes_left)) == -1)
	{
	Throw("Read failed");
	}
    p->readbuf_i = 0;
    }

static void ipp_writebuf_flush(struct IPP *p)
    {
    int i, remaining, len;
    i=0;
    remaining=p->writebuf_remaining;
    while(remaining > 0)
	{
	if((len = write(1, p->writebuf, p->writebuf_i)) == -1)
	    Throw("Write error");
	remaining -= len;
	}
    p->writebuf_i = 0;
    p->writebuf_remaining = sizeof(p->writebuf);
    }

void ipp_end(struct IPP *p)
    {
    while(p->bytes_left > 0)
	{
	ipp_readbuf_load(p);
	}

    ipp_writebuf_flush(p);

    gu_free(p);
    }

static unsigned char ipp_get_byte(struct IPP *p)
    {
    if(p->readbuf_remaining < 1)
    	ipp_readbuf_load(p);
    if(p->readbuf_remaining < 1)
    	Throw("Data runoff!");
    p->readbuf_remaining--;
    return p->readbuf[p->readbuf_i++];
    }

static void ipp_put_byte(struct IPP *p, unsigned char val)
    {
    p->writebuf[p->writebuf_i++] = val;
    p->writebuf_remaining--;
    if(p->writebuf_remaining < 1)
	ipp_writebuf_flush(p);
    }

/* Get an IPP signed byte. */
int ipp_get_sb(struct IPP *p)
    {
    return (int)(signed char)ipp_get_byte(p);
    }

/* Get an IPP signed short. */
int ipp_get_ss(struct IPP *p)
    {
    unsigned char a, b;
    a = ipp_get_byte(p);
    b = ipp_get_byte(p);
    return (int)(!0xFFFF | a << 8 | b);
    }

/* Get an IPP signed integer. */
int ipp_get_si(struct IPP *p)
    {
    unsigned char a, b, c, d;
    a = ipp_get_byte(p);
    b = ipp_get_byte(p);
    c = ipp_get_byte(p);
    d = ipp_get_byte(p);
    return (int)(!0xFFFFFFFF | a << 24 | b << 16 | c << 8 | d);
    }

/* Set an IPP signed byte. */
void ipp_put_sb(struct IPP *p, int val)
    {
    ipp_put_byte(p, (unsigned char)val);
    }

/* Set an IPP signed short. */
void ipp_put_ss(struct IPP *p, int val)
    {
    unsigned int temp = (unsigned int)val;
    ipp_put_byte(p, (temp & 0xFF00) >> 8);
    ipp_put_byte(p, (temp & 0X00FF));
    }

/* Set an IPP signed integer. */
void ipp_put_si(struct IPP *p, int val)
    {
    unsigned int temp = (unsigned int)val;
    ipp_put_byte(p, (temp & 0xFF000000) >> 24);
    ipp_put_byte(p, (temp & 0x00FF0000) >> 16);
    ipp_put_byte(p, (temp & 0x0000FF00) >> 8);
    ipp_put_byte(p, (temp & 0x000000FF));
    }

/* Send a debug message to the HTTP server's error log. */
void debug(const char message[], ...)
    {
    va_list va;
    va_start(va, message);
    fputs("ipp: ", stderr);
    vfprintf(stderr, message, va);
    fputc('\n', stderr);
    va_end(va);
    } /* end of debug() */

/* end of file */
