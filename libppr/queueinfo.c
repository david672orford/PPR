/*
** mouse:~ppr/src/libppr/queueinfo.c
** Copyright 1995--2003, Trinity College Computing Center.
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
** Last modified 15 January 2003.
*/

#include "before_system.h"
#include "gu.h"
#include "global_defines.h"
#include "queueinfo.h"

struct QI {
    enum QUEUEINFO_TYPE type;
    void *name;
    void *comment;
    void *ppdfile;
    gu_boolean transparent_mode;

    };

void *queueinfo_new(enum QUEUEINFO_TYPE qit, const char name[])
    {
    struct QI *qip = gu_alloc(1, sizeof(struct QI));
    
    qip->type = qit;
    qip->name = gu_pcs_new_cstr(name);
    qip->name = gu_pcs_new();
    qip->comment = gu_pcs_new();
    qip->ppdfile = gu_pcs_new();
    qip->transparent_mode = FALSE;

    if(qit == QUEUEINFO_ALIAS)
	{

	}



    return (void *)qip;
    }
    
void queueinfo_delete(void *qip)
    {

    gu_free(qip);
    }
    
/* end of file */
