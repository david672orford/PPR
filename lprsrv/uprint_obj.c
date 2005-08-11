/*
** mouse:~ppr/src/lprsrv/uprint_obj.c
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
** Last modified 9 August 2005.
*/

#include "config.h"
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include "gu.h"
#include "global_defines.h"
#include "rfc1179.h"
#include "lprsrv.h"

/* This is the global variable which the uprint routines
   set when they detect an error. */
int uprint_errno = 0;

/* For uprint_lpq(): */
const char *uprint_arrest_interest_interval = (const char *)NULL;

/*
** Create a new UPRINT object and return a pointer to it.
*/
void *uprint_new(const char *fakername, int argc, const char *argv[])
	{
	struct UPRINT *u;

	DODEBUG_UPRINT(("uprint_new(fakername\"%s\", argc=%d, argv=%p)", fakername, argc, argv));

	/* Allocate space for the UPRINT object: */
	if((u = (struct UPRINT *)calloc(1, sizeof(struct UPRINT))) == (struct UPRINT *)NULL)
		{
		uprint_errno = UPE_MALLOC;
		return (void*)NULL;
		}

	u->signiture = UPRINT_SIGNITURE;

	u->fakername = fakername;
	u->argc = argc;
	u->argv = argv;

	u->dest = (const char *)NULL;
	u->files = (const char **)NULL;

	u->uid = (uid_t)-1;					/* !!! uid may be unsigned !!! */
	u->user = (const char *)NULL;
	u->from_format = (const char *)NULL;
	u->lpr_mailto = (const char *)NULL;
	u->lpr_mailto_host = (const char *)NULL;
	u->fromhost = (const char *)NULL;
	u->proxy_class = (const char *)NULL;
	u->lpr_class = (const char *)NULL;
	u->jobname = (const char *)NULL;
	u->pr_title = (const char *)NULL;

	u->content_type_lp = (const char *)NULL;
	u->content_type_lpr = '\0';
	u->copies = -1;						/* unset as yet */
	u->banner = FALSE;
	u->nobanner = FALSE;
	u->filebreak = TRUE;
	u->priority = -1;					/* unset as yet */
	u->immediate_copy = TRUE;

	u->form = (const char *)NULL;
	u->charset = (const char *)NULL;
	u->width = (const char *)NULL;
	u->length = (const char *)NULL;
	u->indent = (const char *)NULL;
	u->lpi = (const char *)NULL;
	u->cpi = (const char *)NULL;
	u->troff_1 = (const char *)NULL;
	u->troff_2 = (const char *)NULL;
	u->troff_3 = (const char *)NULL;
	u->troff_4 = (const char *)NULL;

	u->lp_filter_modes = (char *)NULL;
	u->lp_interface_options = (char *)NULL;
	u->lp_pagelist = (const char *)NULL;
	u->lp_handling = (const char *)NULL;

	u->osf_LT_inputtray = (const char *)NULL;
	u->osf_GT_outputtray = (const char *)NULL;
	u->osf_O_orientation = (const char *)NULL;
	u->osf_K_duplex = (const char *)NULL;
	u->nup = 0;

	u->unlink = FALSE;
	u->show_jobid = FALSE;
	u->notify_email = FALSE;
	u->notify_write = FALSE;

	DODEBUG_UPRINT(("uprint_new() = %p", u));

	return (void*)u;
	} /* end of uprint_new() */

int uprint_delete(void *p)
	{
	struct UPRINT *upr = (struct UPRINT *)p;

	DODEBUG_UPRINT(("uprint_delete(p = %p)", p));

	if(!upr || upr->signiture != UPRINT_SIGNITURE)
		{
		uprint_errno = UPE_BADARG;
		uprint_error_callback("uprint_delete(): bad object pointer %p (sig %X)", p, ((struct UPRINT *)p)->signiture);
		return -1;
		}

	if(upr->lp_interface_options)
		free(upr->lp_interface_options);
	if(upr->lp_filter_modes)
		free(upr->lp_filter_modes);

	upr->signiture = 0;
	free(p);

	return 0;
	} /* end of uprint_delete() */

static int opt_append(char **p, const char *newmatter)
	{
	int len = 0;

	if(*p != (char*)NULL)
		len = strlen(*p);
	if(len) len++;
	len += strlen(newmatter);
	len++;
	if((*p = (char*)realloc(*p, len)) == (char*)NULL)
		{
		uprint_errno = UPE_MALLOC;
		uprint_error_callback("opt_append(): realloc() failed, errno=%d (%s)\n", errno, gu_strerror(errno));
		return -1;
		}
	strcat(*p, " ");
	strcat(*p, newmatter);
	return 0;
	}

const char *uprint_set_dest(void *p, const char *dest)
	{
	DODEBUG_UPRINT(("uprint_set_dest(p=%p, dest=\"%s\")", p, dest == (const char *)NULL ? "NULL" : dest));
	((struct UPRINT *)p)->dest = dest;
	return dest;
	} /* end of uprint_set_dest() */

const char *uprint_get_dest(void *p)
	{
	DODEBUG_UPRINT(("uprint_get_dest(p=%p) = \"%s\")", p,
		 ((struct UPRINT *)p)->dest == (const char *)NULL ? "NULL" : ((struct UPRINT *)p)->dest));
	return ((struct UPRINT *)p)->dest;
	} /* end of uprint_get_dest() */

int uprint_set_files(void *p, const char *files[])
	{
	#ifdef DEBUG
	{
	int x;
	printf("uprint_set_files(p=%p, *files[]={", p);
	for(x=0; files[x] != (const char *)NULL; x++)
		{
		if(x) printf(", ");
		printf("\"%s\"", files[x]);
		}
	printf("})\n");
	}
	#endif

	if(p == (void*)NULL || files == (const char **)NULL)
		{
		uprint_errno = UPE_BADARG;
		return -1;
		}

	((struct UPRINT *)p)->files = files;

	return 0;
	} /* end of uprint_set_files() */

const char *uprint_set_user(void *p, uid_t uid, gid_t gid, const char *user)
	{
	DODEBUG_UPRINT(("uprint_set_user(p=%p, uid=%ld, user=\"%s\")", p, (long int)uid, user));
	((struct UPRINT *)p)->uid = uid;
	((struct UPRINT *)p)->gid = gid;
	((struct UPRINT *)p)->user = user;
	return user;
	}

const char *uprint_get_user(void *p)
	{
	DODEBUG_UPRINT(("uprint_get_user(p=%p) = \"%s\")", p,
		 ((struct UPRINT *)p)->user == (const char *)NULL ? "NULL" : ((struct UPRINT *)p)->user));
	return ((struct UPRINT *)p)->user;
	}

const char *uprint_set_from_format(void *p, const char *from_format)
	{
	DODEBUG_UPRINT(("uprint_set_from_format(p=%p, from_format=\"%s\")", p, from_format));
	((struct UPRINT *)p)->from_format = from_format;
	return from_format;
	}

/* Set the user name to which email should be sent.  Notice
   that this does not request that email actually be sent.
   You must call uprint_set_notify_email() for that. */
const char *uprint_set_lpr_mailto(void *p, const char *lpr_mailto)
	{
	DODEBUG_UPRINT(("uprint_set_lpr_mailto(p=%p, lpr_mailto=\"%s\")", p, lpr_mailto));
	((struct UPRINT *)p)->lpr_mailto = lpr_mailto;
	return lpr_mailto;
	}

/* Set the hostname which should be used when sending email
   to the user. */
const char *uprint_set_lpr_mailto_host(void *p, const char *lpr_mailto_host)
	{
	DODEBUG_UPRINT(("uprint_set_lpr_mailto_host(p=%p, lpr_mailto_host=\"%s\")", p, lpr_mailto_host));
	((struct UPRINT *)p)->lpr_mailto_host = lpr_mailto_host;
	return lpr_mailto_host;
	}

const char *uprint_set_fromhost(void *p, const char *fromhost)
	{
	DODEBUG_UPRINT(("uprint_set_fromhost(p=%p, fromhost=\"%s\")", p, fromhost));
	((struct UPRINT *)p)->fromhost = fromhost;
	return fromhost;
	}

const char *uprint_set_proxy_class(void *p, const char *proxy_class)
	{
	DODEBUG_UPRINT(("uprint_set_proxy_class(p=%p, proxy_class=\"%s\")", p, proxy_class));
	((struct UPRINT *)p)->proxy_class = proxy_class;
	return proxy_class;
	}

const char *uprint_set_lpr_class(void *p, const char *lpr_class)
	{
	DODEBUG_UPRINT(("uprint_set_lpr_class(p=%p, lpr_class=\"%s\"", p, lpr_class));
	((struct UPRINT *)p)->lpr_class = lpr_class;
	return lpr_class;
	}

const char *uprint_set_jobname(void *p, const char *jobname)
	{
	DODEBUG_UPRINT(("uprint_set_jobname(p=%p, jobname=\"%s\"", p, jobname));

	/* Be careful, an empty job name would cause ppr to balk. */
	if(jobname[0] == '\0')
		{
		uprint_errno = UPE_BADARG;
		return NULL;
		}

	((struct UPRINT *)p)->jobname = jobname;
	return jobname;
	}

const char *uprint_set_pr_title(void *p, const char *pr_title)
	{
	DODEBUG_UPRINT(("uprint_set_title(p=%p, pr_title=\"%s\"", p, pr_title));
	((struct UPRINT *)p)->pr_title = pr_title;
	return pr_title;
	}

/* We don't call the argument "typename" because that upsets C++ compilers. */
const char *uprint_set_content_type_lp(void *p, const char *arg)
	{
	DODEBUG_UPRINT(("uprint_set_content_type_lp(p=%p, arg=\"%s\"", p, arg == (const char *)NULL ? "NULL" : arg));
	((struct UPRINT *)p)->content_type_lp = arg;
	return arg;
	}

int uprint_set_content_type_lpr(void *p, char content_type)
	{
	struct UPRINT *upr = (struct UPRINT *)p;
	DODEBUG_UPRINT(("uprint_set_content_type_lpr(p=%p, content_type=%c)", p, content_type));
	upr->content_type_lpr = content_type;
	return 0;
	}

int uprint_set_copies(void *p, int copies)
	{
	DODEBUG_UPRINT(("uprint_set_copies(p=%p, copies=%d)", p, copies));

	if(copies < -1 || copies > 9999)	/* -1 means don't try to influence */
		{
		DODEBUG_UPRINT(("uprint_set_copies(): unreasonable value for copies"));
		uprint_errno = UPE_BADARG;
		return -1;
		}

	((struct UPRINT *)p)->copies = copies;
	return copies;
	}

gu_boolean uprint_set_banner(void *p, gu_boolean banner)
	{
	DODEBUG_UPRINT(("uprint_set_banner(p=%p, banner=%s)", p, banner ? "TRUE" : "FALSE"));
	((struct UPRINT *)p)->banner = banner;
	return banner;
	}

gu_boolean uprint_set_nobanner(void *p, gu_boolean nobanner)
	{
	DODEBUG_UPRINT(("uprint_set_nobanner(p=%p, nobanner=%s)", p, nobanner ? "TRUE" : "FALSE"));
	((struct UPRINT *)p)->nobanner = nobanner;
	return nobanner;
	}

gu_boolean uprint_set_filebreak(void *p, gu_boolean filebreak)
	{
	DODEBUG_UPRINT(("uprint_set_filebreak(p=%p, filebreak=%s)", p, filebreak ? "TRUE" : "FALSE"));
	((struct UPRINT *)p)->filebreak = filebreak;
	return filebreak;
	}

int uprint_set_priority(void *p, int priority)
	{
	DODEBUG_UPRINT(("uprint_set_priority(p=%p, priority=%d)", p, priority));
	((struct UPRINT *)p)->priority = priority;
	return priority;
	}

gu_boolean uprint_set_immediate_copy(void *p, gu_boolean val)
	{
	DODEBUG_UPRINT(("uprint_set_immediate_copy(p=%p, val=%s)", p, val ? "TRUE" : "FALSE"));
	((struct UPRINT *)p)->immediate_copy = val;
	return val;
	}

const char *uprint_set_form(void *p, const char *formname)
	{
	DODEBUG_UPRINT(("uprint_set_form(p=%p, formname=\"%s\"", p, formname == (const char *)NULL ? "NULL" : formname));
	((struct UPRINT *)p)->form = formname;
	return formname;
	}

const char *uprint_set_charset(void *p, const char *charset)
	{
	DODEBUG_UPRINT(("uprint_set_charset(p=%p, charset=\"%s\"", p, charset));
	((struct UPRINT *)p)->charset = charset;
	return charset;
	}

int uprint_set_length(void *p, const char *length)
	{
	DODEBUG_UPRINT(("uprint_set_length(p=%p, length=\"%s\")", p, length));
	((struct UPRINT *)p)->length = length;
	return 0;
	}

int uprint_set_width(void *p, const char *width)
	{
	DODEBUG_UPRINT(("uprint_set_width(p=%p, width=\"%s\")", p, width));
	((struct UPRINT *)p)->width = width;
	return 0;
	}

int uprint_set_indent(void *p, const char *indent)
	{
	DODEBUG_UPRINT(("uprint_set_indent(p=%p, indent=\"%s\"", p, indent));
	((struct UPRINT *)p)->indent = indent;
	return 0;
	}

int uprint_set_lpi(void *p, const char *lpi)
	{
	DODEBUG_UPRINT(("uprint_set_lpi(p=%p, lpi=\"%s\")", p, lpi));
	((struct UPRINT *)p)->lpi = lpi;
	return 0;
	}

int uprint_set_cpi(void *p, const char *cpi)
	{
	DODEBUG_UPRINT(("uprint_set_cpi(p=%p, cpi=\"%s\")", p, cpi));
	((struct UPRINT *)p)->cpi = cpi;
	return 0;
	}

const char *uprint_set_troff_1(void *p, const char *font)
	{
	DODEBUG_UPRINT(("uprint_set_troff_1(p=%d, font=\"%s\")", p, font));
	((struct UPRINT *)p)->troff_1 = font;
	return font;
	}

const char *uprint_set_troff_2(void *p, const char *font)
	{
	DODEBUG_UPRINT(("uprint_set_troff_2(p=%d, font=\"%s\")", p, font));
	((struct UPRINT *)p)->troff_2 = font;
	return font;
	}

const char *uprint_set_troff_3(void *p, const char *font)
	{
	DODEBUG_UPRINT(("uprint_set_troff_3(p=%d, font=\"%s\")", p, font));
	((struct UPRINT *)p)->troff_3 = font;
	return font;
	}

const char *uprint_set_troff_4(void *p, const char *font)
	{
	DODEBUG_UPRINT(("uprint_set_troff_4(p=%d, font=\"%s\")", p, font));
	((struct UPRINT *)p)->troff_4 = font;
	return font;
	}

/* For this we append. */
const char *uprint_set_lp_interface_options(void *p, const char *lp_interface_options)
	{
	struct UPRINT *upr = (struct UPRINT *)p;
	DODEBUG_UPRINT(("uprint_set_lp_interface_options(p=%d, lp_interface_options=\"%s\")", p, lp_interface_options));
	if(opt_append(&upr->lp_interface_options, lp_interface_options) == -1)
		return (const char *)NULL;
	return upr->lp_interface_options;
	}

/* For this we append. */
const char *uprint_set_lp_filter_modes(void *p, const char *lp_filter_modes)
	{
	struct UPRINT *upr = (struct UPRINT *)p;
	DODEBUG_UPRINT(("uprint_set_lp_filter_modes(p=%d, lp_filter_modes=\"%s\")", p, lp_filter_modes));
	if(opt_append(&upr->lp_filter_modes, lp_filter_modes) == -1)
		return (const char *)NULL;
	return upr->lp_filter_modes;
	}

const char *uprint_set_lp_pagelist(void *p, const char *lp_pagelist)
	{
	DODEBUG_UPRINT(("uprint_set_lp_pagelist(p=%d, lp_pagelist=\"%s\")", p, lp_pagelist));
	((struct UPRINT *)p)->lp_pagelist = lp_pagelist;
	return lp_pagelist;
	}

const char *uprint_set_lp_handling(void *p, const char *lp_handling)
	{
	DODEBUG_UPRINT(("uprint_set_lp_handling(p=%d, lp_handling=\"%s\")", p, lp_handling));
	((struct UPRINT *)p)->lp_handling = lp_handling;
	return lp_handling;
	}

const char *uprint_set_osf_LT_inputtray(void *p, const char *inputtray)
	{
	DODEBUG_UPRINT(("uprint_set_osf_LT_inputtray(p=%d, font=\"%s\")", p, inputtray));
	((struct UPRINT *)p)->osf_LT_inputtray = inputtray;
	return inputtray;
	}

const char *uprint_set_osf_GT_outputtray(void *p, const char *outputtray)
	{
	DODEBUG_UPRINT(("uprint_set_osf_GT_outputtray(p=%d, font=\"%s\")", p, outputtray));
	((struct UPRINT *)p)->osf_GT_outputtray = outputtray;
	return outputtray;
	}

const char *uprint_set_osf_O_orientation(void *p, const char *orientation)
	{
	DODEBUG_UPRINT(("uprint_set_osf_O_orientation(p=%d, font=\"%s\")", p, orientation));
	((struct UPRINT *)p)->osf_O_orientation = orientation;
	return orientation;
	}

const char *uprint_set_osf_K_duplex(void *p, const char *duplex)
	{
	DODEBUG_UPRINT(("uprint_set_osf_K_duplex(p=%d, duplex=\"%s\")", p, duplex));
	((struct UPRINT *)p)->osf_K_duplex = duplex;
	return duplex;
	}

int uprint_set_nup(void *p, int nup)
	{
	DODEBUG_UPRINT(("uprint_set_nup(p=%d, nup=\"%s\")", p, nup));
	((struct UPRINT *)p)->nup = nup;
	return nup;
	}

gu_boolean uprint_set_unlink(void *p, gu_boolean do_unlink)
	{
	DODEBUG_UPRINT(("uprint_set_unlink(p=%p, do_unlink=%s)", p, do_unlink ? "TRUE" : "FALSE"));
	((struct UPRINT *)p)->unlink = do_unlink;
	return do_unlink;
	}

/* Set whether or not email should be sent on normal job completion. */
gu_boolean uprint_set_notify_email(void *p, gu_boolean notify_email)
	{
	DODEBUG_UPRINT(("uprint_set_notify_email(p=%p, notify_email=%s)", p, notify_email ? "TRUE" : "FALSE"));
	((struct UPRINT *)p)->notify_email = notify_email;
	return notify_email;
	}

/* Set whether spooler should write to the user's terminal
   when the job is done. */
gu_boolean uprint_set_notify_write(void *p, gu_boolean notify_write)
	{
	DODEBUG_UPRINT(("uprint_set_notify_write(p=%p, notify_write=%s)", p, notify_write ? "TRUE" : "FALSE"));
	((struct UPRINT *)p)->notify_write = notify_write;
	return notify_write;
	}

gu_boolean uprint_set_show_jobid(void *p, gu_boolean show_jobid)
	{
	DODEBUG_UPRINT(("uprint_set_show_jobid(p=%p, show_jobid=%s)", p, show_jobid ? "TRUE" : "FALSE"));
	((struct UPRINT *)p)->show_jobid = show_jobid;
	return show_jobid;
	}

/* end of file */

