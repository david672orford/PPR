/*
** mouse:~ppr/src/libgu/gu_timeval.c
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
** Last modified 17 January 2002.
*/

#include "config.h"
#include <sys/time.h>
#include "gu.h"

/*! \file
	\brief real-world clock routines
	
These are utility routines for dealing with real-world clock values (struct
timeval).  They provide a way to zero, copy, add, subtract, and compare such
values.

*/

/** Compare two times in struct timeval format

This utility function is used to compare times.  It returns 1 if t1 is
greater than t2, zero if t1 is equal to t2, and -1 if t1 is less than
t2.

*/
int gu_timeval_cmp(const struct timeval *t1, const struct timeval *t2)
	{
	if(t1->tv_sec > t2->tv_sec)
		return 1;
	if(t1->tv_sec < t2->tv_sec)
		return -1;
	if(t1->tv_usec > t2->tv_usec)
		return 1;
	if(t1->tv_usec < t2->tv_usec)
		return -1;
	return 0;
	}

/** Subtract one time in struct timeval format from another

This utility function is used to subtract time t2 from time t1.

*/
void gu_timeval_sub(struct timeval *t1, const struct timeval *t2)
	{
	t1->tv_sec -= t2->tv_sec;
	t1->tv_usec -= t2->tv_usec;
	if(t1->tv_usec < 0)
		{
		t1->tv_usec += 1000000;
		t1->tv_sec--;
		}
	}

/** Add one time in struct timeval format to another

This utility function adds t2 to t1.

*/
void gu_timeval_add(struct timeval *t1, const struct timeval *t2)
	{
	t1->tv_sec += t2->tv_sec;
	t1->tv_usec += t2->tv_usec;
	if(t1->tv_usec > 1000000)
		{
		t1->tv_usec -= 1000000;
		t1->tv_sec++;
		}
	}

/** Copy a struct timeval

This utility function copies time t2 to time t1.

*/
void gu_timeval_cpy(struct timeval *t1, const struct timeval *t2)
	{
	t1->tv_sec = t2->tv_sec;
	t1->tv_usec = t2->tv_usec;
	}

/*
** This utility function zeros a time.
*/
void gu_timeval_zero(struct timeval *t)
	{
	t->tv_sec = 0;
	t->tv_usec = 0;
	}

/* end of file */
