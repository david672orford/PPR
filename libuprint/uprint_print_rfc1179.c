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
** Last modified 12 February 2003.
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
static int do_control_file(void *p, struct REMOTEDEST *scratchpad, const char *local_nodename, int lpr_queueid, int sockfd, const char **files_list)
    {
    const char function[] = "do_control_file";
    struct UPRINT *upr = (struct UPRINT *)p;
    char command[64];		/* command scratch space */
    int code;			/* result byte from remote system */
    char control_file[10000];
    char file_type;
    int copies;
    int copy;
    int i;
    int x;
    const char *filename;

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
    snprintf(control_file, sizeof(control_file), "H%s\nP%s\n", local_nodename, upr->user);
    i = strlen(control_file);

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

	if(upr->jobname)		/* if specified, */
	    {
	    jobname = upr->jobname;
	    }
	else if(strcmp(files_list[0], "-") == 0)
	    {
	    jobname = "stdin";
	    }
	else				/* if not specified, */
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
	    snprintf(&control_file[i], sizeof(control_file) - i, "8PPR --responder %.16s\n8PPR --responder-address %.128s\n", upr->ppr_responder, upr->ppr_responder_address);
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
	if(strcmp(filename, "-") == 0)
	    filename = "";

	if( (i + (12 + strlen(local_nodename)) * copies + 14 + strlen(local_nodename) + strlen(filename) + 1) > sizeof(control_file))
	    {
	    uprint_errno = UPE_INTERNAL;
	    uprint_error_callback("%s(): out of space in control file buffer", function);
	    /* Cancel the job: */
	    uprint_lpr_send_cmd(sockfd, "\1\n", 2);
	    return -1;
	    }
	for(copy = 0; copy < copies; copy++)
	    {
	    snprintf(&control_file[i], sizeof(control_file) - i, "%cdfA%03d.%03d%s\n", file_type, lpr_queueid, x, local_nodename);
	    i += strlen(&control_file[i]);
	    }
	snprintf(&control_file[i], sizeof(control_file) - i, "N%s\nUdfA%03d.%03d%s\n", filename, lpr_queueid, x, local_nodename);
	i += strlen(&control_file[i]);
    	}

    /* Tell the other end that we want to send the control file: */
    snprintf(command, sizeof(command), "\002%d cfA%03d%s\n", (int)strlen(control_file), lpr_queueid, local_nodename);
    if(uprint_lpr_send_cmd(sockfd, command, strlen(command)) == -1)
	{
	return -1;
	}

    /* Check if response is favourable. */
    if((code = uprint_lpr_response(sockfd, TIMEOUT_HANDSHAKE)) == -1)
	{
	uprint_error_callback(_("(Connection lost while negotiating to send control file failed.)"));
	return -1;
	}
    else if(code)
    	{
    	uprint_error_callback(_("Remote LPR/LPD system does not have room for control file."));
	uprint_errno = UPE_TEMPFAIL;
    	return -1;
    	}

    /* Send the control file. */
    if(uprint_lpr_send_cmd(sockfd, control_file, i) == -1)
    	return -1;

    /* Send the zero byte */
    command[0] = '\0';
    if(uprint_lpr_send_cmd(sockfd, command, 1) == -1)
    	return -1;

    /* Check if response if favourable. */
    if((code = uprint_lpr_response(sockfd, TIMEOUT_HANDSHAKE)) == -1)
	{
	uprint_error_callback(_("(Connection lost while transmitting control file.)"));
	return -1;
	}
    else if(code)
    	{
    	uprint_error_callback(_("Remote LPR/LPD system \"%s\" denies correct receipt of control file."), scratchpad->node);
	uprint_errno = UPE_TEMPFAIL;
	uprint_lpr_send_cmd(sockfd, "\1\n", 2);		/* cancel the job */
    	return -1;
    	}

    return 0;
    } /* end of control file */

/*
** Send the data files.
*/
static int do_data_files(void *p, struct REMOTEDEST *scratchpad, const char *local_nodename, int lpr_queueid, int sockfd, const char **files_list)
    {
    const char function[] = "do_data_files";
    struct UPRINT *upr = (struct UPRINT *)p;
    char command[64];		/* command scratch space */
    int code;			/* result byte from remote system */
    int x;
    const char *filename;	/* file we are working on */

    for(x = 0; (filename = files_list[x]) != (const char *)NULL; x++)
	{
	int pffd = -1;	/* file being printed */
	int df_length;	/* length of file being printed */

	/* STDIN */
	if(strcmp(filename, "-") == 0)
	    {
	    /* copy stdin to a temporary file */
	    if((pffd = uprint_file_stdin(&df_length)) == -1)
		{
		/* Failed, cancel the job: */
		uprint_lpr_send_cmd(sockfd, "\1\n", 2);
		return -1;
		}
	    }

	/* Disk file */
	else
	    {
	    uid_t saved_euid;
	    struct stat statbuf;
	    int saved_errno;

	    /* We will be switching to the real id in a moment: */
	    saved_euid = geteuid();

	    /* Try */
	    uprint_errno = UPE_NONE;
	    do  {
		/* Switch to the real id indicated in the UPRINT structure: */
		seteuid(0);
		if(seteuid(upr->uid) == -1)
		    {
		    uprint_errno = UPE_INTERNAL;
		    uprint_error_callback("%s(): seteuid(%ld) failed, errno=%d (%s)", function, (long)upr->uid, errno, gu_strerror(errno));
		    break;
		    }

		/* Try to open the file to be printed: */
		pffd = open(filename, O_RDONLY);
		saved_errno = errno;

		/* Switch back to the id for "ppr": */
		seteuid(0);
		if(seteuid(saved_euid) == -1)
		    {
		    uprint_errno = UPE_INTERNAL;
		    uprint_error_callback("%s(): seteuid(%ld) failed, errno=%d (%s)", function, (long)saved_euid, saved_errno, gu_strerror(saved_errno));
		    break;
		    }

		/* If the open() failed, */
		if(pffd == -1)
		    {
		    uprint_errno = UPE_NOFILE;
		    uprint_error_callback(_("Can't open \"%s\", errno=%d (%s)."), filename, errno, gu_strerror(errno));
		    break;
		    }

		/* Use fstat() to determine the file's length: */
		if(fstat(pffd, &statbuf) == -1)
		    {
		    uprint_errno = UPE_INTERNAL;
		    uprint_error_callback("%s(): fstat() failed, errno=%d (%s)", function, errno, gu_strerror(errno));
		    break;
		    }
	    	df_length = (int)statbuf.st_size;
	        } while(FALSE);

	    /* Catch */
	    if(uprint_errno != UPE_NONE)
	    	{
		/* Cancel the job: */
		uprint_lpr_send_cmd(sockfd, "\1\n", 2);
		return -1;
	    	}
	    } /* end of if disk file */

	/* Ask permission to send the data file: */
	snprintf(command, sizeof(command), "\003%d dfA%03d.%03d%s\n", df_length, lpr_queueid, x, local_nodename);
	if(uprint_lpr_send_cmd(sockfd, command, strlen(command)) == -1)
	    {
	    close(pffd);
	    return -1;
	    }

	/* Check if response is favourable: */
	if((code = uprint_lpr_response(sockfd, TIMEOUT_HANDSHAKE)) == -1)
	    {
	    uprint_error_callback(_("(Connection lost while negotiating to send data file.)"));
	    close(pffd);				/* couldn't read response */
	    return -1;
	    }
        else if(code)
	    {
	    uprint_error_callback(_("Remote LPR/LPD system \"%s\" does not have room for data file."), scratchpad->node);
	    uprint_errno = UPE_TEMPFAIL;
	    close(pffd);				/* close print file */
	    uprint_lpr_send_cmd(sockfd, "\1\n", 2);	/* cancel the job */
	    return -1;
	    }

	/* Send and close the data file and
	   catch any error in sending the data file. */
	{
	int ret = uprint_lpr_send_data_file(pffd, sockfd);
	close(pffd);
	if(ret == -1)
	    return -1;
	}

	/* Send the zero byte */
	command[0] = '\0';
	if(uprint_lpr_send_cmd(sockfd, command, 1) == -1)
	    return -1;

	/* Check if response if favourable. */
	if((code = uprint_lpr_response(sockfd, TIMEOUT_PRINT)) == -1)
	    {
	    uprint_error_callback(_("(Connection lost while sending data file.)"));
	    return -1;
	    }
	else if(code)
	    {
	    uprint_error_callback(_("Remote LPR/LPD system \"%s\" denies correct receipt of data file."), scratchpad->node);
	    uprint_errno = UPE_TEMPFAIL;
	    uprint_lpr_send_cmd(sockfd, "\1\n", 2);	/* cancel the job */
	    return -1;
	    }
	} /* end of for each file */

    return x;
    }

/*
** Dispatch the job using the LPR/LPD protocol described
** in RFC 1179.
*/
int uprint_print_rfc1179(void *p, struct REMOTEDEST *scratchpad)
    {
    const char function[] = "uprint_print_rfc1179";
    struct UPRINT *upr = (struct UPRINT *)p;
    const char *local_nodename;	/* this node */
    int lpr_queueid;		/* queue id of this new job */
    int sockfd;			/* connexion to other node */
    char command[64];		/* command scratch space */
    int code;			/* remote command result code */
    const char **files_list;	/* list of files to be printed */
    const char *default_files_list[] = {"-", NULL};
    int files_count = 0;	/* number of files sent */
    int retcode;

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

    /* Make sure user is filled in: */
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

    /* Get our nodename: */
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

    /* Connect to the remote system: */
    if((sockfd = uprint_lpr_make_connection_with_failover(scratchpad->node)) == -1)
	return -1;

    /* Try block: */
    retcode = 0;
    while(TRUE)
	{
        /* Say we want to send a job: */
        snprintf(command, sizeof(command), "\002%s\n", scratchpad->printer);
        if(uprint_lpr_send_cmd(sockfd, command, strlen(command)) == -1)
            {
            retcode = -1;
            break;
            }

        /* Check if the response if favorable: */
        if((code = uprint_lpr_response(sockfd, TIMEOUT_HANDSHAKE)) == -1)
            {
	    uprint_error_callback(_("(Connection lost while negotiating to send job.)"));
            retcode = -1;
            break;
            }
        else if(code)
            {
            uprint_error_callback(_("Remote LPR/LPD system \"%s\" refuses to accept job for \"%s\" (%d)."), scratchpad->node, scratchpad->printer, code);
            uprint_errno = UPE_DENIED;
            retcode = -1;
            break;
            }

        /* Generate a queue id.  Notice that the effective
           user is "ppr" (or more precisely, the owner of
           /etc/ppr/uprint.conf) when we call this. */
        if((lpr_queueid = uprint_lpr_nextid()) == -1)
            {
            retcode = -1;
            break;
            }

        /* Find the files list: */
        if(upr->files)
            files_list = upr->files;
        else
            files_list = default_files_list;

        /* Build and send the control file: */
        if(do_control_file(p, scratchpad, local_nodename, lpr_queueid, sockfd, files_list) == -1)
            {
            retcode = -1;
            break;
            }

        /* Send each data file: */
        if((files_count = do_data_files(p, scratchpad, local_nodename, lpr_queueid, sockfd, files_list)) == -1)
            {
            retcode = -1;
            break;
            }

	/* end of Try block */
	break;
	}

    /* Close the connexion to the remote system. */
    close(sockfd);

    if(upr->show_jobid && files_count > 0)
	printf("job id is %s-0 (%d file%s)\n", upr->dest, files_count, files_count > 1 ? "s" : "");

    return retcode;
    } /* end of uprint_print_rfc1179() */

/* end of file */
