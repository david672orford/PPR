/*
** mouse:~ppr/src/LICENSE.txt
** Copyright 1995--2005, Trinity College Computing Center.
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
** Last modified 16 March 2005.
*/

#include "config.h"
#include "gu.h"
#include "global_defines.h"
#include "libppr_font.h"

struct FONT_INFO *font_info_new(void)
	{
	struct FONT_INFO *p = (struct FONT_INFO *)gu_alloc(1, sizeof(struct FONT_INFO));
	p->font_family = p->font_weight = p->font_slant = p->font_width =
		p->font_psname = p->font_encoding = p->ascii_subst_font = NULL;
	p->font_type = 0;
	return p;
	}

void font_info_delete(struct FONT_INFO *p)
	{
	gu_free_if(p->font_family);
	gu_free_if(p->font_weight);
	gu_free_if(p->font_slant);
	gu_free_if(p->font_width);
	gu_free_if(p->font_psname);
	gu_free_if(p->font_encoding);
	gu_free_if(p->ascii_subst_font);
	gu_free(p);
	}


