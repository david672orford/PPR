/*
** mouse:~ppr/src/uprint/uprint_print_rfc1179.c
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
** Last modified 20 February 2003.
*/

#include "before_system.h"
#include <stdarg.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "uprint.h"
#include "uprint_private.h"

/*
** TIMEOUT_HANDSHAKE is the time to wait on operations that
** should be completed immediately, such as stating the desired
** queue name.  TIMEOUT_PRINT is the amount of time to wait for
** a response to the zero byte sent at the end of a data file.
** TIMEOUT_PRINT can be set separately because some printer
** Ethernet boards don't respond right away.
*/
#define TIMEOUT_HANDSHAKE 30
#define TIMEOUT_PRINT 30

/*
** Build and send the control file.
*/
static char *create_control_file(void *p, struct REMOTEDEST *scratchpad, const char *local_nodename, int lpr_queueid, const char **files_list)
	{
	const char function[] = "create_control_file";
	struct UPRINT *upr = (struct UPRINT *)p;
	char file_type;
	int copies;
	int copy;
	int i;
	int x;
	const char *filename;
	char control_file[10000];

	/* I am not sure if we should refrain in the
	   presence of solaris extensions or not. */
	if(!scratchpad->solaris_extensions)
		{
		uprint_parse_lp_interface_options(p);
		uprint_parse_lp_filter_modes(p);
		}

	/* If the file type can't be determined, use "formatted file": */
	if((file_type = uprint_get_content_type_lpr(p)) == '\0')
		file_type = 'f';

	/* If number of copies unspecified (-1), use 1: */
	if((copies = upr->copies) < 0)
		copies = 1;

	/* Build the `control file', start with nodename and user: */
	i = 0;
	snprintf(&control_file[i], sizeof(control_file) - i, "H%s\n", local_nodename);
	i += strlen(&control_file[i]);
	snprintf(&control_file[i], sizeof(control_file) - i, "P%s\n", upr->user);
	i += strlen(&control_file[i]);

	/* Mail on job completion? */
	if(upr->notify_email)
		{
		snprintf(&control_file[i], sizeof(control_file) - i, "M%.*s\n", LPR_MAX_M, upr->lpr_mailto ? upr->lpr_mailto : upr->user);
		i += strlen(&control_file[i]);
		}

	/* Now add "burst page" information: */
	if(! upr->nobanner)
		{
		const char *lpr_class = upr->lpr_class ? upr->lpr_class : local_nodename;
		const char *jobname;

		if(upr->jobname)				/* if specified, */
			{
			jobname = upr->jobname;
			}
		else if(strcmp(files_list[0], "-") == 0)
			{
			jobname = "stdin";
			}
		else							/* if not specified, */
			{
			if((jobname = strrchr(files_list[0], '/')) != (const char *)NULL)
				jobname++;
			else
				jobname = files_list[0];
			}

		snprintf(&control_file[i], sizeof(control_file) - i, "J%.*s\nC%.*s\nL%.*s\n",
				LPR_MAX_J, jobname,
				LPR_MAX_C, lpr_class,
				LPR_MAX_L, upr->user);

		i += strlen(&control_file[i]);
		}

	/* If it needs a title for pr, */
	if(file_type == 'p' && upr->pr_title != (const char *)NULL)
		{
		snprintf(&control_file[i], sizeof(control_file) - i, "T%.*s\n", LPR_MAX_T, upr->pr_title);
		i += strlen(&control_file[i]);
		}

	/* Width: */
	if(upr->width)
		{
		snprintf(&control_file[i], sizeof(control_file) - i, "W%.*s\n", LPR_MAX_W, upr->width);
		i += strlen(&control_file[i]);
		}

	/* Indent: */
	if(upr->indent)
		{
		snprintf(&control_file[i], sizeof(control_file) - i, "I%.*s\n", LPR_MAX_I, upr->indent);
		i += strlen(&control_file[i]);
		}

	/* Troff fonts: */
	if(file_type == 't' || file_type == 'n')
		{
		if(upr->troff_1)
			{
			snprintf(&control_file[i], sizeof(control_file) - i, "1%.*s", LPR_MAX_1, upr->troff_1);
			i += strlen(&control_file[i]);
			}
		if(upr->troff_2)
			{
			snprintf(&control_file[i], sizeof(control_file) - i, "2%.*s", LPR_MAX_2, upr->troff_2);
			i += strlen(&control_file[i]);
			}
		if(upr->troff_3)
			{
			snprintf(&control_file[i], sizeof(control_file) - i, "3%.*s", LPR_MAX_3, upr->troff_3);
			i += strlen(&control_file[i]);
			}
		if(upr->troff_4)
			{
			snprintf(&control_file[i], sizeof(control_file) - i, "4%.*s", LPR_MAX_4, upr->troff_4);
			i += strlen(&control_file[i]);
			}
		}

	/* Possibly add OSF lpr extensions information: */
	if(scratchpad->osf_extensions)
		{
		if(upr->osf_LT_inputtray)
			{
			snprintf(&control_file[i], sizeof(control_file) - i, "<%.*s\n", LPR_MAX_DEC, upr->osf_LT_inputtray);
			i += strlen(&control_file[i]);
			}

		if(upr->osf_GT_outputtray)
			{
			snprintf(&control_file[i], sizeof(control_file) - i, ">%.*s\n", LPR_MAX_DEC, upr->osf_GT_outputtray);
			i += strlen(&control_file[i]);
			}

		if(upr->osf_O_orientation)
			{
			snprintf(&control_file[i], sizeof(control_file) - i, "O%.*s\n", LPR_MAX_DEC, upr->osf_O_orientation);
			i += strlen(&control_file[i]);
			}

		if(upr->osf_K_duplex)
			{
			snprintf(&control_file[i], sizeof(control_file) - i, "K%.*s\n", LPR_MAX_DEC, upr->osf_K_duplex);
			i += strlen(&control_file[i]);
			}

		if(upr->nup > 0)
			{
			snprintf(&control_file[i], sizeof(control_file) - i, "G%d\n", upr->nup);
			i += strlen(&control_file[i]);
			}
		} /* end of if osf_extensions */

	/* Possibly add Solaris extensions information: */
	if(scratchpad->solaris_extensions)
		{
		if(upr->form)
			{
			snprintf(&control_file[i], sizeof(control_file) - i, "5f%.*s\n", LPR_MAX_5f, upr->form);
			i += strlen(&control_file[i]);
			}
		if(upr->lp_interface_options)
			{
			snprintf(&control_file[i], sizeof(control_file) - i, "O%.*s\n", LPR_MAX_O, upr->lp_interface_options);
			i += strlen(&control_file[i]);
			}
		if(upr->lp_filter_modes)
			{
			snprintf(&control_file[i], sizeof(control_file) - i, "5y%.*s\n", LPR_MAX_5y, upr->lp_filter_modes);
			i += strlen(&control_file[i]);
			}
		if(upr->lp_pagelist)
			{
			snprintf(&control_file[i], sizeof(control_file) - i, "5P%.*s\n", LPR_MAX_5P, upr->lp_pagelist);
			i += strlen(&control_file[i]);
			}
		if(upr->charset)
			{
			snprintf(&control_file[i], sizeof(control_file) - i, "5S%.*s\n", LPR_MAX_5S, upr->charset);
			i += strlen(&control_file[i]);
			}
		if(upr->content_type_lp)
			{
			snprintf(&control_file[i], sizeof(control_file) - i, "5T%.*s\n", LPR_MAX_5T, upr->content_type_lp);
			i += strlen(&control_file[i]);
			}
		} /* end of solaris_extensions */

	/* Possibly add PPR lpr extensions information: */
	if(scratchpad->ppr_extensions)
		{
		if(upr->ppr_responder && upr->ppr_responder_address && strchr(upr->ppr_responder_address, '@'))
			{
			snprintf(&control_file[i], sizeof(control_file) - i, "8PPR --responder %.16s\n", upr->ppr_responder);
			i += strlen(&control_file[i]);
			snprintf(&control_file[i], sizeof(control_file) - i, "8PPR --responder-address %.128s\n", upr->ppr_responder_address);
			i += strlen(&control_file[i]);

			if(upr->ppr_responder_options)
				{
				snprintf(&control_file[i], sizeof(control_file) - i, "8PPR --responder-options %.128s\n", upr->ppr_responder_options);
				i += strlen(&control_file[i]);
				}
			}
		} /* end of ppr_extensions */

	/* Add each of the file names to the control file: */
	for(x = 0; (filename = files_list[x]) != (const char *)NULL; x++)
		{
		int expected_size, saved_i = i;
		
		if(strcmp(filename, "-") == 0)
			filename = "";

		/*
		** ldfA123.000mouse\n
		** ^   ^   ^  ^
		** |   |   |  +-- local_nodename
		** |	   +----- x
		** |   +--------- lpr_queueid
		** +------------- file_type
		**
		** N/etc/profile\n
		** ^^
		** |+------------ filename
		** +------------- indicates actual filename
		**
		** UdfA123.000mouse\n
		** ^
		** +------------- undicates file to unlink when done
		*/
		expected_size = (12 + strlen(local_nodename)) * copies
						+ 2 + strlen(filename)
						+ 12 + strlen(local_nodename)
						;
		if( (i + expected_size + 1) > sizeof(control_file))
			{
			uprint_errno = UPE_INTERNAL;
			uprint_error_callback("%s(): out of space in control file buffer", function);
			return NULL;
			}

		/* This emmits lines which specify the type of the file and the name that it should
		   have in the queue directory.  This is repeated to produce multiple copies.
		   */
		for(copy = 0; copy < copies; copy++)
			{
			snprintf(&control_file[i], sizeof(control_file) - i, "%cdfA%03d.%03d%s\n", file_type, lpr_queueid, x, local_nodename);
			i += strlen(&control_file[i]);
			}

		/* N specifies the name of the source file. */
		snprintf(&control_file[i], sizeof(control_file) - i, "N%s\n", filename);
		i += strlen(&control_file[i]);

		/* U specifies the file to unlink. */
		snprintf(&control_file[i], sizeof(control_file) - i, "UdfA%03d.%03d%s\n", lpr_queueid, x, local_nodename);
		i += strlen(&control_file[i]);

		/* Make sure our computation above was correct. */
		if(!((i - saved_i) == expected_size))
			{
			uprint_errno = UPE_INTERNAL;
			uprint_error_callback("%s(): assertion failed: ((i - saved_i) = %d) == (expected_size = %d)", function, (i - saved_i), expected_size);
			return NULL;
			}
		}

	/* printf("control_file[] = \"%s\"\n", control_file); */

	/* Copy it into memory which we can return. */
	return gu_strdup(control_file);
	} /* end of control file */

/*
** Dispatch the job using the LPR/LPD protocol described
** in RFC 1179.
*/
int uprint_print_rfc1179(void *p, struct REMOTEDEST *scratchpad)
	{
	const char function[] = "uprint_print_rfc1179";
	struct UPRINT *upr = (struct UPRINT *)p;
	const char *local_nodename; /* this node */
	int lpr_queueid;			/* queue id of this new job */
	char lpr_queueid_str[10];
	const char **files_list;	/* list of files to be printed */
	const char *default_files_list[] = {"-", NULL};
	char *control_file;
	int files_count = 0;		/* number of files sent */
	const char **args = (const char**)NULL;

	DODEBUG(("%s(p=%p, scratchpad=%p)", function, p, scratchpad));
	DODEBUG(("scratchpad={node=\"%s\", printer=\"%s\", osf_extensions=%s, solaris_extensions=%s, ppr_extensions=%s}",
		scratchpad->node, scratchpad->printer,
		scratchpad->osf_extensions ? "TRUE" : "FALSE",
		scratchpad->solaris_extensions ? "TRUE" : "FALSE",
		scratchpad->ppr_extensions ? "TRUE" : "FALSE"));

	/* The destination queue must be specified. */
	if(! upr->dest)
		{
		uprint_error_callback("%s(): dest not specified", function);
		uprint_errno = UPE_NODEST;
		return -1;
		}

	if(! scratchpad)
		{
		uprint_error_callback("%s(): scratchpad is NULL", function);
		uprint_errno = UPE_BADARG;
		return -1;
		}

	/* Check that the information from /etc/ppr/uprint-remote.conf
	   has been filled in. */
	if(scratchpad->node[0] == '\0' || scratchpad->printer[0] == '\0')
		{
		uprint_error_callback("%s(): printdest_claim_rfc1179() must suceed first", function);
		uprint_errno = UPE_BADORDER;
		return -1;
		}

	/* Make sure username is filled in. */
	if(! upr->user)
		{
		uprint_error_callback("%s(): user is NULL", function);
		uprint_errno = UPE_BADARG;
		return -1;
		}

	/* Make sure the files list (if specified) has
	   at least one member. */
	if(upr->files && !upr->files[0])
		{
		uprint_error_callback("%s(): file list is empty", function);
		uprint_errno = UPE_BADARG;
		return -1;
		}

	/* Get our nodename to state as the job source. */
	if(upr->fromhost)
		{
		local_nodename = upr->fromhost;
		}
	else if((local_nodename = uprint_lpr_nodename()) == (char*)NULL)
		{
		uprint_errno = UPE_INTERNAL;
		uprint_error_callback("%s(): uprint_lpr_nodename() failed", function);
		return -1;
		}

	/* Check not make sure nothing is too long: */
	if(strlen(local_nodename) > LPR_MAX_H || strlen(upr->user) > LPR_MAX_P)
		{
		uprint_errno = UPE_BADARG;
		uprint_error_callback(_("%s(): local nodename or username is too long"), function);
		return -1;
		}

	/* Generate a queue id.  Notice that the effective
	   user is "ppr" (or more precisely, the owner of
	   /etc/ppr/uprint.conf) when we call this. */
	if((lpr_queueid = uprint_lpr_nextid()) == -1)
		{
		return -1;
		}
	snprintf(lpr_queueid_str, sizeof(lpr_queueid_str), "%d", lpr_queueid);

	/* Find the files list. */
	if(upr->files)
		files_list = upr->files;
	else
		files_list = default_files_list;

	/* Build and send the control file. */
	if(!(control_file = create_control_file(p, scratchpad, local_nodename, lpr_queueid, files_list)))
		{
		return -1;
		}

	/* Create the argument list for uprint_rfc1179 (our setuid root helper). */
	{
	int si, di;
	int args_available = 10;
	args = (const char**)gu_alloc(args_available, sizeof(char**));
	di = 0;
	args[di++] = UPRINT_RFC1179;
	args[di++] = "print";
	args[di++] = scratchpad->node;
	args[di++] = scratchpad->printer;
	args[di++] = local_nodename;
	args[di++] = upr->user;
	args[di++] = lpr_queueid_str;	 
	args[di++] = control_file;
	for(si = 0; files_list[si]; si++)
		{
		if(di >= (args_available - 1))
			{
			args_available += 4;
			/* printf("expanding args[] to %d members\n", args_available); */
			args = (const char**)gu_realloc(args, args_available, sizeof(char**));
			}
		args[di++] = files_list[si];
		}
	args[di++] = NULL;
	}

	{
	int retcode;

	retcode = uprint_run_rfc1179(UPRINT_RFC1179, args);

	gu_free((char*)control_file);
	gu_free(args);

	/* If the command suceeded, and we have been asked to show the job ID, and at least one
	   file was printed, then show a fake jobid.
	   */
	if(retcode == 0 && upr->show_jobid && files_count > 0)
		printf("job id is %s-0 (%d file%s)\n", upr->dest, files_count, files_count > 1 ? "s" : "");

	return retcode;
	}
	} /* end of uprint_print_rfc1179() */

/* end of file */
