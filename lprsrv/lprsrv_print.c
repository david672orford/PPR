/*
** mouse:~ppr/src/lprsrv/lprsrv_print.c
** Copyright 1995--2004, Trinity College Computing Center.
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
** Last modified 5 February 2004.
*/

/*
** This module contains functions to execute the LPD protocol receive
** print job command.  It accepts the control and data files and then
** uses libuprint to send the job to the correct spooler.
*/

#include "before_system.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <stdlib.h>
#include "gu.h"
#include "global_defines.h"
#include "lprsrv.h"
#include "uprint.h"

struct DATA_FILE
		{
		off_t start;			/* offset in temp file */
		size_t length;			/* size in bytes */
		const char *Name;		/* origional name of the file */
		char type;				/* type of this file */
		int copies;				/* number of copies */
		} ;

static void clear_data_files(struct DATA_FILE data_files[], int n)
	{
	int x;
	for(x = 0; x < n; x++)
		{
		data_files[x].start = 0;
		data_files[x].length = 0;
		data_files[x].Name = (const char *)NULL;
		data_files[x].type = 0;
		data_files[x].copies = 0;
		}
	}

/*==================================================================
** Receive and print files
==================================================================*/

/*
** Open the temporary file to hold the data file or files.
** (If we store multiple files in it we will remember the
** starting offset and length of each.)
*/
static int open_tmp(void)
	{
	const char function[] = "open_tmp";
	char fname[MAX_PPR_PATH];
	int file;

	ppr_fnamef(fname, "%s/ppr-lprsrv-%ld-XXXXXX", TEMPDIR, (long)getpid());
	DODEBUG_PRINT(("%s(): creating \"%s\"", function, fname));

	if((file = mkstemp(fname)) == -1)
		{
		debug("%s(): mkstemp(\"%s\") failed, errno=%d (%s)", function, fname, errno, gu_strerror(errno));
		return -1;
		}

	DODEBUG_GRITTY(("%s(): opened \"%s\", handle=%d", function, fname, file));

	unlink(fname);						/* we don't need the name any more */

	return file;						/* return pointer to file stream */
	} /* end of open_tmp() */

/*
** Read file data from stdin and write it to the
** temporary file.
**
** Return the number of bytes read.  Return -1 if there
** is an error.
*/
static ssize_t receive_data_file(size_t size_of_file, int tempfile)
	{
	gu_boolean readerror = FALSE;
	gu_boolean diskfull = FALSE;
	unsigned int free_files, free_blocks;
	unsigned int q_free_files, q_free_blocks;

	DODEBUG_PRINT(("receive_data_file(size_of_file=%ld, tempfile=%d", (long)size_of_file, tempfile));

	/* Get disk space, die if we can't. */
	if(disk_space(TEMPDIR, &free_blocks, &free_files) != 0
				|| disk_space(QUEUEDIR, &q_free_blocks, &q_free_files) != 0 )
		{
		fputc(1, stdout);
		fflush(stdout);
		fatal(1, "receive_data_file(): disk_space() failed");
		}

	DODEBUG_DISKSPACE(("free_blocks=%d, free_files=%d, q_free_blocks=%d, q_free_files=%d",
		free_blocks, free_files, q_free_blocks, q_free_files));

	/* If the disk space discovered by the call to disk_space() is
	   more than a specified minimum, */
	if(free_files > MIN_INODES
		&& free_blocks > ((size_of_file+511)/512 + MIN_BLOCKS)
		&& q_free_files > MIN_INODES
		&& q_free_blocks > ((size_of_file+511)/512 + MIN_BLOCKS) )
		{
		fputc(0, stdout);		/* say we have space */
		fflush(stdout);			/* to receive the file */
		}
	else
		{
		debug("Insufficient disk space to receive %d byte file", size_of_file);
		fputc(2, stdout);		/* say we don't have space */
		fflush(stdout);			/* to receive the file */
		return -1;
		}

	/* Copy the file */
	{
	size_t remaining = size_of_file;
	int toread, towrite, written;
	unsigned char buffer[8192];
	while(remaining && !readerror)
		{
		DODEBUG_GRITTY(("%ld bytes left to read", (long)remaining));

		if(remaining > sizeof(buffer))
			toread = sizeof(buffer);
		else
			toread = remaining;

		/* Read a block from stdin */
		DODEBUG_GRITTY(("attempting to read %d bytes", toread));
		towrite = fread(buffer, sizeof(unsigned char), toread, stdin);
		DODEBUG_GRITTY(("%d bytes read", towrite));

		if(towrite==0)			/* If end of file or error, */
			{					/* (In this case, end of file is an error.) */
			debug("receive_data_file(): error reading from stdin");
			readerror = TRUE;
			}
		else
			{
			if(!diskfull)
				{
				if( (written=write(tempfile,buffer,towrite)) == -1 )
					fatal(1, "receive_data_file(): write(%d, ,%d) failed, errno=%d",tempfile,towrite,errno);

				if(written < towrite)
					{
					debug("receive_data_file(): disk full");
					diskfull = TRUE;
					}
				}
			remaining -= towrite;				/* subtract what we wrote */
			}
		} /* end of data reading loop */
	}

	DODEBUG_GRITTY(("done, readerror=%s, diskfull=%s", readerror ? "TRUE" : "FALSE",
		diskfull ? "TRUE" : "FALSE"));

	/* If file not recieved correctly, */
	if(readerror || fgetc(stdin) != 0)
		{
		fputc(1, stdout);		/* say so */
		fflush(stdout);			/* flush the one */
		return -1;
		}

	/* If ran out of disk space, */
	if(diskfull)
		{						/* hope that the other end will */
		fputc(2, stdout);		/* understand a code of 2 at this point. */
		fflush(stdout);
		return -1;
		}

	/* If no error, */
	else
		{
		fputc(0, stdout);		/* say so by writing a 0 byte */
		fflush(stdout);			/* and flushing the zero. */
		return size_of_file;
		}
	} /* end of receive_date_file() */

/*
** Interpret a PPR option from the control file.
*/
static void handle_ppr_option(void *upr, const char *option)
	{
	if(strncmp(option, "--responder ", 12) == 0)
		uprint_set_ppr_responder(upr, option+12);
	else if(strncmp(option, "--responder-address ", 20) == 0)
		uprint_set_ppr_responder_address(upr, option+20);
	else if(strncmp(option, "--responder-options ", 20) == 0)
		uprint_set_ppr_responder_options(upr, option+20);
	}

/*
** Interpret a Solaris option from the control file.
*/
static void handle_solaris_option(void *upr, const char *option)
	{
	switch(option[0])
		{
		case 'f':
			uprint_set_form(upr, option+1);
			break;
		case 'H':
			uprint_set_lp_handling(upr, option+1);
			break;
		case 'O':
			uprint_set_lp_interface_options(upr, option+1);
			break;
		case 'P':
			uprint_set_lp_pagelist(upr, option+1);
			break;
		case 'S':
			uprint_set_charset(upr, option+1);
			break;
		case 'T':
			uprint_set_content_type_lp(upr, option+1);
			break;
		case 'y':
			uprint_set_lp_filter_modes(upr, option+1);
			break;
		}
	}

/*
** Read a control file and remember the important information.
*/
static void receive_control_file(int control_file_len, struct DATA_FILE data_files[], int *files_unlinked, void *upr)
	{
	#define MAX_NAME_CONSIDER 40
	char last_file_name[MAX_NAME_CONSIDER+1] = {'\0'};
	static char *control_buffer = (char *)NULL;
	int control_buffer_len = 0;
	int N_Count = 0;
	int files_count = 0;

	DODEBUG_PRINT(("receive_control_file(): length is %d bytes", control_file_len));

	/* Make sure we have a buffer big enough to store
	   the complete text of the control file.  Notice
	   that the buffer is always one byte bigger than
	   control_buffer_len indicates it is.  This is to
	   leave room for a terminating NULL byte.
	   */
	if(control_buffer_len < control_file_len)
		{
		if(control_buffer) gu_free(control_buffer);
		control_buffer = (char*)gu_alloc(control_file_len + 1, sizeof(char));
		control_buffer_len = control_file_len;
		}

	/* Tell the client that we have room for the control file: */
	fputc(0, stdout);
	fflush(stdout);

	/* Read the control file into memory: */
	{
	char *ptr;
	int justread, toread;

	for(ptr = control_buffer, toread = control_file_len; toread > 0; ptr += justread, toread -= justread)
		{
		if((justread = read(0, ptr, toread)) == -1)
			fatal(1, "receive_control_file(): read() failed, errno=%d (%s)", errno, gu_strerror(errno));
		}

	control_buffer[control_buffer_len] = '\0';
	}

	/* Do final handshaking. */
	if(fgetc(stdin) == 0)		/* Read zero byte. */
		fputc(0, stdout);		/* If we got it, acknowledge, */
	else						/* otherwise, */
		fputc(1, stdout);		/* deny. */

	fflush(stdout);

	/* If the L line does not appear, that means suppress banner page. */
	uprint_set_nobanner(upr, TRUE);

	/* Take the control file lines one-by-one: */
	{
	char *line;
	int linelen;

	for(line = control_buffer; *line; line += (linelen + 1), line += strspn(line, "\n\r"))
		{
		linelen = strcspn(line, "\n\r");
		line[linelen] = '\0';

		DODEBUG_CONTROL_FILE(("control file line: %s", line));

		switch(line[0])
			{
			case 'P':							/* User identification (a required line) */
				uprint_set_user(upr, (uid_t)-1, (gid_t)-1, line+1);
				break;

			case 'H':							/* Host identification (a required line) */
				uprint_set_fromhost(upr, line+1);
				break;

			case 'C':							/* Class for banner page */
				uprint_set_lpr_class(upr, line+1);
				break;

			case 'I':							/* Indent */
				uprint_set_indent(upr, line+1);
				break;

			case 'J':							/* Job name for banner page */
				uprint_set_jobname(upr, line+1);
				break;

			case 'L':							/* User name for banner page */
				uprint_set_nobanner(upr, FALSE);
				/* gu_strlcpy(control->Lbanner, line+1, LPR_MAX_L); */
				break;

			case 'M':							/* User to mail to when complete */
				uprint_set_lpr_mailto(upr, line+1);
				uprint_set_notify_email(upr, TRUE);
				break;

			case 'N':							/* Original name of job file */
				if(N_Count < MAX_FILES_PER_JOB)
					{
					/* Some LPR clients use " " for stdin */
					if(strcmp(line+1, " ") == 0)
						data_files[N_Count++].Name = "standard input";
					else
						data_files[N_Count++].Name = line+1;
					}
				break;

			case 'T':							/* Title for PR */
				uprint_set_pr_title(upr, line+1);
				break;

			case 'W':							/* Width of output */
				uprint_set_width(upr, line+1);
				break;

			case 'U':							/* Name of file to unlink when done */
				if(line[1] == 'd')				/* <-- filter out spurious lines from */
					(*files_unlinked)++;		/*	   lpr's -r switch */
				break;

			case '1':
				uprint_set_troff_1(upr, line+1);
				break;

			case '2':
				uprint_set_troff_2(upr, line+1);
				break;

			case '3':
				uprint_set_troff_3(upr, line+1);
				break;

			case '4':
				uprint_set_troff_4(upr, line+1);
				break;

			case '5':
				handle_solaris_option(upr, line+1);
				break;

			case '8':
				if(strncmp(line, "8PPR ", 5) == 0)
					handle_ppr_option(upr, line+5);
				break;

			case '<':							/* DEC OSF input tray */
				uprint_set_osf_LT_inputtray(upr, line+1);
				break;

			case 'K':							/* DEC OSF duplex */
				uprint_set_osf_K_duplex(upr, line+1);
				break;

			case 'G':							/* DEC OSF N-Up */
				uprint_set_nup(upr, atoi(line+1));
				break;

			case '>':							/* DEC OSF output tray */
				uprint_set_osf_GT_outputtray(upr, line+1);
				break;

			case 'O':							/* DEC OSF orientation */
				if(strcmp(line+1, "landscape") == 0 || strcmp(line+1, "portrait") == 0)
					uprint_set_osf_O_orientation(upr, line+1);
				else
					handle_solaris_option(upr, line);
				break;

			case 'f':							/* files of various types */
			case 'l':
			case 'o':
			case 'p':
			case 'r':
			case 'c':
			case 'g':
			case 'v':
			case 'n':
			case 'd':
			case 't':
			case 'x':
				if(line[1] != '/')				/* if not spurious line, */
					{
					if(strncmp(line+1, last_file_name, MAX_NAME_CONSIDER) == 0) /* if same as last name */
						{														/* it means extra copy */
						data_files[files_count - 1].copies++;
						DODEBUG_CONTROL_FILE(("(%d copies)", data_files[files_count - 1].copies));
						}
					else if(files_count < MAX_FILES_PER_JOB)
						{
						data_files[files_count].type = *line;
						data_files[files_count].copies = 1;
						files_count++;
						gu_strlcpy(last_file_name, line+1, MAX_NAME_CONSIDER);
						}
					else
						{
						debug("no room for file");
						}
					}
				break;

			default:							/* Ignore others */
				break;
			}

		} /* end of line reading loop */
	} /* end of context for line reading loop */

	DODEBUG_PRINT(("receive_control_file(): done"));
	} /* end of receive_control_file() */

/*
** Run the spooler submission command indicated by prog, pass it the
** arguments indicated by args, and pass it the data file indicated
** by n.
**
** The run_uid and run_gid are the user ID and group ID which the child should switch to.
*/
static void dispatch_files_run(uid_t run_uid, gid_t run_gid, const char *prog, const char *args[], int tempfile, off_t start, size_t length)
	{
	const char function[] = "dispatch_files_run";
	pid_t pid;							/* process id of PPR or LP */
	int fds[2];							/* file descriptors of pipe */

	/* This code is used to test for problems in the
	   build_argv*() functions called above. */
	#ifdef DEBUG_PRINT
	{
	FILE *logfile;
	if((logfile = fopen(LPRSRV_LOGFILE, "a")) != (FILE*)NULL)
		{
		int x;
		fprintf(logfile, "%s(run_uid=%ld, prog=\"%s\", args={", function, (long int)run_uid, prog);
		for(x=0; args[x]; x++)
			{
			if(x)
				fputs(", ", logfile);
			fprintf(logfile, "\"%s\"", args[x]);
			}
		fprintf(logfile, "}, start=%lu, length=%lu)\n", (long unsigned)start, (long unsigned)length);
		fclose(logfile);
		}
	}
	#endif

	/* Go to the right place in the temporary file: */
	if(lseek(tempfile, start, SEEK_SET) == -1)
		fatal(1, "%s(): lseek() failed", function);

	/* Open a pipe which will be used to connect us to the child: */
	if(pipe(fds) == -1)
		fatal(1, "%s(): pipe() failed, errno=%d (%s)", function, errno, gu_strerror(errno) );

	/* Keep trying until we can fork() a child. */
	while((pid = fork()) == -1)
		{
		debug("%s(): fork() failed, retry in 60 seconds", function);
		sleep(60);
		}

	/* Here we fork.  The child process will execute PPR or LP or LPR. */
	if(pid)						/* if parent */
		{
		int wstat;
		char buffer[4096];
		int readlen, written, thiswrite;

		/* Close our copy of the read end of the pipe: */
		close(fds[0]);

		/* Copy the required amount to the child: */
		readlen = written = 0;
		do	{
			if(written==readlen)
				{
				if((readlen = read(tempfile, buffer, length > sizeof(buffer) ? sizeof(buffer) : length)) == -1)
					fatal(1, "%s(): read() failed, errno=%d (%s)", function, errno, gu_strerror(errno) );

				if(readlen == 0)
					fatal(1, "%s(): defective temp file?", function);

				written = 0;
				length -= readlen;
				}

			if( (thiswrite = write(fds[1], buffer+written, readlen-written)) == -1 )
				fatal(1, "%s(): write() failed, errno=%d (%s)", function, errno, gu_strerror(errno) );
			written += thiswrite;
			} while(readlen > written || length > 0);

		/* If we don't do this ppr/lpr/lp will wait forever: */
		close(fds[1]);

		/* Wait for PPR or LP/LPR to terminate. */
		DODEBUG_PRINT(("%s(): waiting for %s to exit...", function, prog));
		wait(&wstat);

		if(WCOREDUMP(wstat))
			{
			debug("%s(): %s dumped core", function, prog);
			}
		else if(WIFEXITED(wstat))
			{
			switch(WEXITSTATUS(wstat))
				{
				case 0:
					DODEBUG_PRINT(("%s(): %s ran normally", function, prog));
					break;
				case 240:
					debug("%s(): Child can't open log file", function);
					break;
				case 241:
					debug("%s(): setuid(0) failed in child", function);
					break;
				case 242:
					debug("%s(): setgid(%ld) failed in child", function, (long int)run_gid);
					break;
				case 243:
					debug("%s(): setuid(%ld) failed in child", function, (long int)run_uid);
					break;
				case 244:
					debug("%s(): setregid(%ld, %ld) failed in child", function, (long int)run_gid, (long int)run_gid);
					break;
				case 245:
					debug("%s(): setreuid(%ld, %ld) failed in child", function, (long int)run_uid, (long int)run_uid);
					break;
				case 246:
					debug("%s(): setuid(0) did not fail in child", function, (long int)run_uid, (long int)run_uid);
					break;
				case 247:
					debug("%s(): Exec() of %s failed", function, prog);
					break;
				default:
					debug("%s(): %s exited with code %d", function, prog, WEXITSTATUS(wstat));
					break;
				}
			}
		else
			{
			debug("%s(): %s terminated by signal %d ***", function, prog, WTERMSIG(wstat));
			}
		}

	/*------------------------------------------------------------
	** Child process.  Execute ppr, lpr, or lp as selected above.
	**----------------------------------------------------------*/
	else						/* child process */
		{
		int log;				/* We will open the lprsrv log file with this */

		close(fds[1]);			/* close our copy of write end */

		dup2(fds[0], 0);		/* Connect read end of pipe */
		close(fds[0]);			/* to stdin. */

		/* Open the lprsrv log file */
		if((log = open(LPRSRV_LOGFILE, O_WRONLY | O_APPEND | O_CREAT, UNIX_644)) == -1)
			_exit(240);

		/* Connect stdout and stderr to the log file. */
		dup2(log, 1);
		dup2(log, 2);
		close(log);				/* We don't need this descriptor any more. */

		/* Fully relinquish root authority and become designated user. */
		if(setuid(0) == -1)
			_exit(241);
		if(setgid(run_gid) == -1)
			_exit(242);
		if(setuid(run_uid) == -1)
			_exit(243);
		if(setregid(run_gid, run_gid) == -1)
			_exit(244);
		if(setreuid(run_uid, run_uid) == -1)
			_exit(245);
		if(run_uid != 0 && setuid(0) != -1)
			_exit(246);
		execv(prog, (char **)args);

		_exit(247);				/* exit here if exec failed */
		} /* end of if child */

	} /* end of dispatch_files_run() */

/*
** Send the data files to the proper spooler.
*/
static void dispatch_files(int tempfile, struct DATA_FILE *data_files, int file_count, void *upr, const char printer[], int spooler, const char *prog, const char fromhost[], const struct ACCESS_INFO *access_info)
	{
	#define MAX_PRINT_ARGV 100
	const char *args[MAX_PRINT_ARGV+3];			/* space to build command line */
	uid_t uid_to_use;							/* uid to run spooler program as */
	gid_t gid_to_use;
	const char *proxy_class = (const char *)NULL;
	int args_used = 0;							/* arguments filled in already */
	int findex;									/* index of file we are working on */
	int last_set;								/* last findex which had copies and type set */

	/* DODEBUG_PRINT(("dispatch_files(tempfile=%d, data_files=%p, file_count=%d, upr=%p, fromhost=\"%s\", access_info=%p)", tempfile, data_files, file_count, upr, fromhost, access_info)); */
	DODEBUG_PRINT(("dispatch_files()"));

	/* Choose local user to run as and possibly choose proxy mode. */
	get_proxy_identity(&uid_to_use, &gid_to_use, &proxy_class, fromhost, uprint_get_user(upr), printdest_claim_ppr(printer), access_info);
	if(proxy_class)
		uprint_set_proxy_class(upr, proxy_class);

	for(findex=last_set=0; findex < file_count; findex++)
		{
		int i;
		struct DATA_FILE *data = &data_files[findex];

		DODEBUG_PRINT(("dispatch_files(): dispatching file number %d", findex));

		/* If first time thru loop or copies or type have changed, */
		if(findex == 0 || data->copies != data_files[last_set].copies || data->type != data_files[last_set].type)
			{
			uprint_set_copies(upr, data->copies);
			uprint_set_content_type_lpr(upr, data->type);
			last_set = findex;

			switch(spooler)
				{
				case 1:
					args_used = uprint_print_argv_ppr(upr, args, MAX_PRINT_ARGV);
					break;
				case 2:
					args_used = uprint_print_argv_bsd(upr, args, MAX_PRINT_ARGV);
					break;
				case 3:
					args_used = uprint_print_argv_sysv(upr, args, MAX_PRINT_ARGV);
					break;
				default:
					fatal(1, "%s line %d: missing case", __FILE__, __LINE__);
				}
			}
		i = args_used;

		if(prog == PPR_PATH)
			{
			if(data->Name)
				{
				args[i++] = "--lpq-filename";
				args[i++] = data->Name[0] ? data->Name : "stdin";
				}

			/* If debugging is on, send ppr error messages to stderr too: */
			#ifdef DEBUG_PRINT
			args[i++] = "-e";
			args[i++] = "both";
			#endif
			}

		args[i] = (const char *)NULL;
		dispatch_files_run(uid_to_use, gid_to_use, prog, args, tempfile, data->start, data->length);
		} /* end of for() loop */

	DODEBUG_PRINT(("dispatch_files(): done"));
	} /* end of dispatch_files() */

/*
** Function to execute the ^B command:
** (The ^B command receives a print job.)
*/
void do_request_take_job(const char printer[], const char fromhost[], const struct ACCESS_INFO *access_info)
	{
	const char function[] = "do_request_take_job";
	void *upr = (void*)NULL;			/* pointer to uprint object */
	int tempfile = -1;					/* file handle of temp file */
	int files_on_hand = 0;
	int files_to_unlink = 0;
	struct DATA_FILE data_files[MAX_FILES_PER_JOB];

	int spooler;								/* number of spooler to use */
	const char *prog;							/* pathname of spooler program */

	/* Build a command line appropriate for the spooler: */
	if(printdest_claim_ppr(printer))
		{
		spooler = 1;
		prog = PPR_PATH;
		}

	else if(printdest_claim_bsd(printer))
		{
		spooler = 2;
		prog = uprint_path_lpr();
		}

	else if(printdest_claim_sysv(printer))
		{
		spooler = 3;
		prog = uprint_path_lp();
		}

	/* If queue not found, */
	else
		{
		fputc(1, stdout);
		fflush(stdout);
		return;
		}

	/* If we make it this far, respond favourably. */
	fputc(0, stdout);
	fflush(stdout);

	/* Initialize the data structure which keeps track of the data files. */
	clear_data_files(data_files, MAX_FILES_PER_JOB);

	/*
	** Subcommand loop: read lines with
	** subcommands in them and process them.
	*/
	{
	char line[256];						/* line buffer */
	gu_boolean aborted = FALSE;			/* TRUE if ^A subcommand received */
	while(! aborted && fgets(line, sizeof(line), stdin))
		{
		line[strcspn(line, "\n\r")] = '\0';

		switch(line[0])
			{
			case 1:						/* Abort job */
				DODEBUG_PRINT(("abort job"));
				aborted = TRUE;
				break;

			case 2:						/* control file */
				DODEBUG_PRINT(("control file: %s", &line[1]));

				/* Structure to store UPRINT job information: */
				if((upr = uprint_new("lprsrv", 0, (const char **)NULL)) == (void*)NULL)
					fatal(1, "%s(): uprint_new() failed", function);

				/* Set the printer: */
				uprint_set_dest(upr, printer);

				/* Set the from format: */
				uprint_set_from_format(upr, access_info->ppr_from_format);

				/* Override email host. */
				uprint_set_lpr_mailto_host(upr, fromhost);

				/* Fill uprint information and data_files
				   information from the control file. */
				receive_control_file(atoi(line+1), data_files, &files_to_unlink, upr);

				break;

			case 3:						/* data file */
				DODEBUG_PRINT(("data file: %s", &line[1]));

				/* Don't overflow the array */
				if(files_on_hand >= MAX_FILES_PER_JOB)
					{
					debug("%s(): too many data files", function);
					fputc(2, stdout);
					fflush(stdout);
					break;
					}

				/* Open the temporary file if it is not already open.
				   If we can't open it, say there is no room.
				   Doing this will cause the remote end
				   to try again later.  I don't know if this
				   is a good idea or not.
				   */
				if(tempfile == -1 && (tempfile=open_tmp()) == -1)
					{
					DODEBUG_PRINT(("%s(): open_tmp() failed", function));
					fputc(2, stdout);
					fflush(stdout);
					break;
					}

				/* Store the data file: */
				data_files[files_on_hand].start = lseek(tempfile, (off_t)0, SEEK_CUR);
				data_files[files_on_hand].length = receive_data_file(atoi(&line[1]), tempfile);
				files_on_hand++;
				break;

			default:
				debug("%s(): unreconized subcommand: %c", function, line[0]);
				fputc(2, stdout);		/* Why 2? */
				fflush(stdout);
				break;
			} /* end of switch */

		/*
		** If we have a control file and as many data files
		** as the control file says we should have, go print them.
		*/
		if(! aborted && upr && files_on_hand >= files_to_unlink)
			{
			DODEBUG_PRINT(("%s(): dispatching %d file(s)", function, files_on_hand));
			dispatch_files(tempfile, data_files, files_on_hand, upr, printer, spooler, prog, fromhost, access_info);
			aborted = TRUE;				/* use the abort code to clean up */
			}

		if(aborted)						/* or cleaning up */
			{
			/* get ready for next job */
			if(tempfile != -1)
				{
				close(tempfile);
				tempfile = -1;
				}
			if(upr)
				{
				uprint_delete(upr);
				upr = (void*)NULL;
				}
			files_on_hand = files_to_unlink = 0;
			clear_data_files(data_files, MAX_FILES_PER_JOB);
			}

		} /* end of line reading loop */
	} /* end of line reading context */

	/* If the job was dispatched to the spooler, upr will have been
	   set to NULL. */
	if(upr)
		{
		debug("%s(): bad request, only %d of %d files received", function, files_on_hand, files_to_unlink);
		uprint_delete(upr);
		}

	/* If the temporary file is still open, we received data files
	   without a control file to go with them. */
	if(tempfile != -1)
		{
		debug("%s(): bad request, %d file(s) with no control file", function, files_on_hand);
		close(tempfile);
		}
	} /* end of do_request_take_job() */

/* end of file */

