/*
** mouse:~ppr/src/LICENSE.txt
** Copyright 1995--1999, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appears in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is"
** without express or implied warranty.
**
** Last modified 19 July 1999.
*/

#include "before_system.h"
#include "gu.h"
#include "global_defines.h"

#include "libppr_font.h"

struct FONT_INFO *font_info_new(void)
	{
	struct FONT_INFO *p = (struct FONT_INFO *)gu_alloc(1, sizeof(struct FONT_INFO));
	p->font_family = p->font_weight = p->font_slant = p->font_width =
		p->font_psname = p->font_encoding = p->font_type = p->ascii_subst_font = NULL;
	return p;
	}

void font_info_delete(struct FONT_INFO *p)
	{
	if(p->font_family) gu_free(p->font_family);
	if(p->font_weight) gu_free(p->font_weight);
	if(p->font_slant) gu_free(p->font_slant);
	if(p->font_width) gu_free(p->font_width);
	if(p->font_psname) gu_free(p->font_psname);
	if(p->font_encoding) gu_free(p->font_encoding);
	if(p->font_type) gu_free(p->font_type);
	if(p->ascii_subst_font) gu_free(p->ascii_subst_font);
	gu_free(p);
	}


