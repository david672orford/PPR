/*
** mouse:~ppr/src/pprdrv/ppr-gs.c
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
** Last modified 2 August 2002.
*/

#include "before_system.h"
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include "gu.h"
#include "global_defines.h"

const char myname[] = "ppr-gs";

int main(int argc, char *argv[])
    {
    const char rip_exe[] = "../ppr-gs/bin/gs";
    const char **rip_args;
    int si, di;

    rip_args = gu_alloc(argc + 6, sizeof(const char *));

    for(si=1,di=0; si<argc; si++)
	{
	rip_args[di++] = argv[si];
	}

    rip_args[di++] = "-q";
    rip_args[di++] = "-dSAFER";
    rip_args[di++] = "-sOutputFile=| cat - >&3";
    rip_args[di++] = "-";

    execv(rip_exe, (char**)rip_args);
    fprintf(stderr, "%s: can't exec Ghostscript, errno=%d (%s)\n", myname, errno, gu_strerror(errno));
    return 1;
    } /* end of main() */

/* end of file */
