/*
** mouse:~ppr/src/lprsrv/olprsrv.c
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
** Last modified 10 March 2003.
*/

/*
** This file contains the source code for the olprsrv.	olprsrv is
** basically the LPR/LPD compatible print server which was distributed
** with PPR version 1.30.  It is provided because the lprsrv in PPR
** version 1.32 is a major rewrite.
**
** Notice that this server doesn't redefine libppr_throw().	 This
** isn't worth fixing since this is retired code.  All it means
** is that some fatal error messages get written to stderr rather
** than the log file.
*/

/*
** Berkeley LPR compatible server for PPR and LP on System V Unix.
** There is also partial support for passing jobs to LPR.
*/

#include "before_system.h"
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <ctype.h>
#include <sys/utsname.h>
#include <signal.h>
#include <pwd.h>
#include <grp.h>
#include "gu.h"
#include "global_defines.h"

#include "olprsrv.h"
#include "util_exits.h"
#include "uprint.h"
#include "version.h"

/* Control file information */
struct CONTROL_FILE {
		char Host[LPR_MAX_H+1];					/* Host which sent file (required) */
		char Person[LPR_MAX_P+1];						/* person for whom it is being printed (required) */
		char Class[LPR_MAX_C+1];						/* Class name for banner */
		char Jobname[LPR_MAX_J+1];						/* Job name for banner page */
		int print_banner;						/* TRUE or FALSE */
		char Lbanner[LPR_MAX_L+1];						/* User name for banner page */
		char Mailto[LPR_MAX_M+1];						/* User to send mail to */
		char Title[LPR_MAX_T+1];						/* Title for pr */
		int indent;								/* Indent columns */
		int width;								/* Width of output */
		char LT_inputtray[LPR_MAX_DEC+1];
		char Kduplex[LPR_MAX_DEC+1];
		int Gnup;
		char GT_outputtray[LPR_MAX_DEC+1];
		char Oorientation[LPR_MAX_DEC+1];

		struct
				{
				char Name[LPR_MAX_N+1];			/* origional name of the file */
				char type;						/* type of this file */
				int copies;						/* number of copies */
				} files[MAX_FILES_PER_JOB];

		int files_count;						/* number of different file lines received */
		int N_count;							/* N lines received */

		int files_unlinked;						/* count of unlink lines */

		char mailaddr[LPR_MAX_M+1+LPR_MAX_H+1];			/* fully assembled mail address */
		char principal[LPR_MAX_M+1+LPR_MAX_H+1];		/* fully assembled principal string */
		} ;

struct DATA_FILE
		{
		off_t start;
		int length;
		} ;

/* Variables which tell who we are talking to. */
char client_dns_name[LPR_MAX_H+1];
int client_port;

/* Variables based upon command line options: */
const char *super_root_string = (char*)NULL;
char *arrest_interesting_time = (char*)NULL;

/* Variables for reading lines */
int bytes_read;					/* running total by lprsrv_getline() */
char line[256];					/* place where lprsrv_getline() puts its results */

/* This is set to TRUE in a child process: */
int am_child = FALSE;

/*
** Print an error message and abort.
*/
void fatal(int exitcode, const char *string, ... )
	{
	va_list va;
	FILE *file;

	va_start(va,string);
	if( (file=fopen(LPRSRV_LOGFILE, "a")) != NULL )
		{
		fprintf(file, "FATAL: (%ld, %s) ", (long)getpid(), datestamp() );
		vfprintf(file, string, va);
		fputs("\n", file);
		fclose(file);
		}
	fputs("lprsrv: ", stdout);
	vfprintf(stdout, string, va);
	fputs("\n", stdout);
	va_end(va);

	if(! am_child )
		unlink(LPRSRV_LOCKFILE);

	exit(exitcode);
	} /* end of fatal() */

/*
** Print a debug line in the lprsrv log file.
*/
void debug(char *string, ...)
	{
	va_list va;
	FILE *logfile;

	va_start(va, string);
	if( (logfile=fopen(LPRSRV_LOGFILE, "a")) != NULL )
		{
		fprintf(logfile, "DEBUG: (%ld, %s) ", (long)getpid(), datestamp() );
		vfprintf(logfile, string, va);
		fprintf(logfile, "\n");
		fclose(logfile);
		}
	va_end(va);
	} /* end of debug() */

void uprint_error_callback(const char *format, ...)
	{
	FILE *logfile;
	va_list va;

	if((logfile = fopen(LPRSRV_LOGFILE, "a")) != NULL)
		{
		fprintf(logfile, "UPRINT: (%ld, %s) ", (long)getpid(), datestamp() );

		va_start(va, format);
		vfprintf(logfile, format, va);
		fputc('\n', logfile);
		va_end(va);

		fclose(logfile);
		}
	} /* end of uprint_error_callback() */

#ifdef DEBUG_MAIN
#define DODEBUG_MAIN(a) debug a
#else
#define DODEBUG_MAIN(a)
#endif

#ifdef DEBUG_STANDALONE
#define DODEBUG_STANDALONE(a) debug a
#else
#define DODEBUG_STANDALONE(a)
#endif

#ifdef DEBUG_PRINT
#define DODEBUG_PRINT(a) debug a
#else
#define DODEBUG_PRINT(a)
#endif

#ifdef DEBUG_CONTROL_FILE
#define DODEBUG_CONTROL_FILE(a) debug a
#else
#define DODEBUG_CONTROL_FILE(a)
#endif

#ifdef DEBUG_GRITTY
#define DODEBUG_GRITTY(a) debug a
#else
#define DODEBUG_GRITTY(a)
#endif

#ifdef DEBUG_DISKSPACE
#define DODEBUG_DISKSPACE(a) debug a
#else
#define DODEBUG_DISKSPACE(a)
#endif

#ifdef DEBUG_LPQ
#define DODEBUG_LPQ(a) debug a
#else
#define DODEBUG_LPQ(a)
#endif

#ifdef DEBUG_LPRM
#define DODEBUG_LPRM(a) debug a
#else
#define DODEBUG_LPRM(a) debug a
#endif

/*
** Run an external command.	 Return 0 if it runs an exits with
** a status of zero, return -1 if it can't be run or exits
** with a non-zero status.
*/
int run(const char *file, const char *const argv[])
	{
	pid_t pid;
	int wstatus;

	if( (pid = fork()) == -1 )
		{
		debug("run(): fork() failed, errno=%d (%s)", errno, gu_strerror(errno));
		return -1;
		}
	else if( pid == 0 )
		{
		execv(file, (char *const *)argv); /* it's OK, execv()  won't modify it */
		_exit(242);
		}
	else
		{
		if(wait(&wstatus) == -1)
			{
			debug("run(): wait() failed, errno=%d (%s)", errno, gu_strerror(errno) );
			return -1;
			}
		else
			{
			if( ! WIFEXITED(wstatus) )
				{
				debug("run(): didn't exit");
				return -1;
				}
			if( WEXITSTATUS(wstatus) )
				{
				debug("run(): command failed");
				return -1;
				}
			debug("run(): sucess");
			return 0;
			}
		}
	#ifdef GNUC_HAPPY
	return 0;
	#endif
	} /* end of run() */

/*
** Do a truncating string copy.	 The parameter maxlen
** specifies the maximun length to copy exclusive of the
** NULL which terminates the string.
*/
void clipcopy(char *dest, const char *source, int maxlen)
	{
	while(maxlen-- && *source)
		*(dest++) = *(source++);

	*dest = (char)NULL;
	} /* end of clipcopy() */

/*
** Read a line from the client and remove the
** line terminator.
**
** This routine increments the global variable "bytes_read".
*/
int lprsrv_getline(void)
	{
	if(fgets(line,sizeof(line),stdin)!=(char*)NULL)
		{
		int len;

		len=strlen(line);

		bytes_read+=len;

		if( (len==0) || (line[len-1]!='\n') )			/* make sure it is */
			{											/* a whole line */
			fatal(1, "unterminated line, aborting");
			}

		while( line[len-1]=='\n' || line[len-1]=='\r' ) /* Remove the line */
			line[--len]=(char)NULL;						/* terminator */

		return TRUE;
		}
	else
		{
		return FALSE;
		}
	} /* end of lprsrv_getline() */

/*
** Try to determine the fully qualified host name of
** a specified computer.
**
** The returned string is in malloc() allocated memory.
*/
static const char *fully_qualify_hostname(const char *hostname)
	{
	struct hostent *hostinfo;
	struct in_addr my_addr;

	if( (hostinfo=gethostbyname(hostname)) == (struct hostent *)NULL )
		{
		fprintf(stderr, "gethostbyname() failed for \"%s\"\n", hostname);
		return (char*)NULL;
		}

	if(hostinfo->h_addrtype != AF_INET)
		{
		fprintf(stderr, "Yours is not an internet address.\n");
		return (char*)NULL;
		}

	memcpy(&my_addr, hostinfo->h_addr_list[0], sizeof(my_addr));

	if( (hostinfo=gethostbyaddr((char*)&my_addr, sizeof(my_addr), AF_INET)) == (struct hostent *)NULL )
		{
		fprintf(stderr, "gethostbyaddr() failed for %s\n", inet_ntoa(my_addr));
		return (char*)NULL;
		}

	return gu_strdup(hostinfo->h_name);
	} /* end of fully_qualify_hostname() */

/*
** Try to determine the fully qualified host name of this computer.
*/
static const char *get_full_hostname(void)
	{
	struct utsname system_info;

	if(uname(&system_info) == -1)
		{
		fprintf(stderr, "uname() failed, errno=%d (%s)\n", errno, gu_strerror(errno));
		return (char*)NULL;
		}

	return fully_qualify_hostname(system_info.nodename);
	} /* end of get_full_hostname() */

/*
** This function examines a list of colon-separated host names which
** may have been supplied with the -S switch.  If the indicated host
** name is in the list then this function returns TRUE.
*/
int is_super_root(const char *hostname)
	{
	int answer = FALSE;
	int hnlen = strlen(hostname);
	const char *ptr;
	int len;

	if(super_root_string == (char*)NULL)
		super_root_string = get_full_hostname();

	if(super_root_string != (char*)NULL)
		{
		for(ptr=super_root_string; *ptr; ptr+=len, ptr+=strspn(ptr,":"))
			{
			if( (len=strcspn(ptr, ":")) == hnlen && strncmp(hostname, ptr, len) == 0 )
				{
				answer = TRUE;
				break;
				}
			}
		}

	return answer;
	} /* end of is_super_root() */

/*
** Return TRUE if the client is authorized to connect.
**
** Note: /etc/hosts.lpd has been extended to accept domain names.
** ie: an entry such as ".eecs.umich.edu" in the hosts.lpd file would
**	   allow all machines in the eecs.umich.edu domain to connect.
** There is now also a /etc/hosts.lpd_deny file; any machine that fits there
** will be rejected (machines listed here take precendence over those
** listed in hosts.lpd)
*/
int _authorized(const char *name, const char *file)
	{
	FILE *f;
	char line[256];
	char *tmp;
	int strlen_name = strlen(name);
	int answer = FALSE;

	if( (f=fopen(file, "r")) != (FILE*)NULL )
		{
		while(fgets(line,sizeof(line),f) != (char*)NULL)
			{
			line[strcspn(line," \t\n")] = (char)NULL;

			for(tmp=line; *tmp; tmp++)
				*tmp = tolower(*tmp);

			if(line[0] == '.')
				{
				int lendiff = strlen_name - strlen(line);
				if(lendiff >= 1)
					{
					if(strcmp(line, &name[lendiff]) == 0)
						{
						answer = TRUE;
						break;
						}
					}
				}
			else
				{
				if(strcmp(line, name) == 0)
					{
					answer = TRUE;
					break;
					}
				}
			}

		fclose(f);
		}

	return answer;
	} /* end of _authorized() */

int authorized(const char *name)
	{
	int answer = FALSE;

	if(_authorized(name, "/etc/hosts.lpd"))
		answer = TRUE;
	else if(_authorized(name, "/etc/hosts.equiv"))
		answer = TRUE;

	/* reject any hosts listed in hosts.lpd_deny */
	if(answer && _authorized(name, "/etc/hosts.lpd_deny"))
		answer = FALSE;

	return answer;
	} /* end of authorized() */

/*==================================================================
** Receive and print files
==================================================================*/

/*
** Read file data from stdin and write it to the
** temporary file.
**
** Return the number of bytes read.	 Return -1 if there
** is an error.
*/
int receive_data_file(char *command, int tempfile)
	{
	size_t size_of_file, remaining;
	int toread, towrite, written;
	unsigned char buffer[8192];
	int readerror = FALSE;
	int diskfull = FALSE;
	unsigned int free_files, free_blocks;
	unsigned int q_free_files, q_free_blocks;

	/* Read the count of how many bytes we will get. */
	size_of_file = remaining = atoi(command);

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
		&& free_blocks > ((remaining+511)/512 + MIN_BLOCKS)
		&& q_free_files > MIN_INODES
		&& q_free_blocks > ((remaining+511)/512 + MIN_BLOCKS) )
		{
		fputc(0, stdout);		/* say we have space */
		fflush(stdout);			/* to receive the file */
		}
	else
		{
		debug("Insufficient disk space to receive %d byte file", remaining);
		fputc(2, stdout);		/* say we don't have space */
		fflush(stdout);			/* to receive the file */
		return -1;
		}

	/* Copy the file */
	while(remaining && !readerror)
		{
		DODEBUG_GRITTY(("%d bytes left to read", remaining));

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
** Clear the control file structure.
*/
void clear_control(struct CONTROL_FILE *control)
		{
		control->Host[0] = (char)NULL;
		control->Person[0] = (char)NULL;
		control->Class[0] = (char)NULL;
		control->Jobname[0] = (char)NULL;
		control->print_banner = FALSE;
		control->Lbanner[0] = (char)NULL;
		control->Mailto[0] = (char)NULL;
		control->Title[0] = (char)NULL;
		control->indent = 0;
		control->width = 0;
		control->files_count = 0;
		control->files_unlinked = 0;
		control->N_count = 0;
		control->files_count = 0;
		control->mailaddr[0] = (char)NULL;
		control->principal[0] = (char)NULL;
		control->LT_inputtray[0] = (char)NULL;
		control->Kduplex[0] = (char)NULL;
		control->Gnup = 0;
		control->GT_outputtray[0] = (char)NULL;
		control->Oorientation[0] = (char)NULL;
		} /* end of clear_control() */

/*
** Read a control file and remember the important information.
*/
void receive_control_file(char *command, struct CONTROL_FILE *control)
	{
	int size = atoi(command);	/* Read the size of the control file from command line */
	#define MAX_NAME_CONSIDER 40
	char last_file_name[MAX_NAME_CONSIDER+1] = {(char)NULL};

	DODEBUG_GRITTY(("Control file is %d bytes long", size));

	fputc(0, stdout);			/* Say we have room for this file. */
	fflush(stdout);				/* (We don't store it, so we always will.) */

	/*
	** Read all the lines until we have gotten out quota of bytes.
	** (That is, until we have gotten the numbers of bytes we have
	** been told to expect to find in the control file.)
	** We try to recognize control file lines and stash the information
	** away in the structure called "control".	When it comes to the
	** type of the file, we may write strings to "control.type"
	** which will later be passed to ppr(1) with its -T switch.
	** We will also at times write strings to "control.lp_type".
	** This will be used if the jobs is eventually submitted to the
	** system spooler.	If the system spooler is LP, "control.lp_type"
	** is submitted as the argument to the -T switch, if the system
	** spooler is LPR, the contents of "control.lp_type" is submited
	** as the switch that it is.
	*/
	for(bytes_read = 0; (bytes_read < size) && lprsrv_getline(); )
		{
		DODEBUG_CONTROL_FILE(("control file line: %s", line));

		switch(*line)
			{
			case 'P':							/* User identification (a required line) */
				clipcopy(control->Person, line+1, LPR_MAX_P);
				break;
			case 'H':							/* Host identification (a required line) */
				clipcopy(control->Host, line+1, LPR_MAX_H);
				break;
			case 'C':							/* Class for banner page */
				clipcopy(control->Class, line+1, LPR_MAX_C);
				break;
			case 'I':							/* Indent */
				control->indent = atoi(line+1);
				break;
			case 'J':							/* Job name for banner page */
				clipcopy(control->Jobname, line+1, LPR_MAX_J);
				break;
			case 'L':							/* User name for banner page */
				control->print_banner = TRUE;
				clipcopy(control->Lbanner, line+1, LPR_MAX_L);
				break;
			case 'M':							/* User to mail to when complete */
				clipcopy(control->Mailto, line+1, LPR_MAX_M);
				break;
			case 'N':							/* Origional name of job file */
				if(control->N_count < MAX_FILES_PER_JOB)
					{
					/* Some LPR clients use " " for stdin */
					if(strcmp(line+1, " ") == 0)
						strcpy(control->files[control->N_count++].Name, "standard input");
					else
						clipcopy(control->files[control->N_count++].Name, line+1, LPR_MAX_N);
					}
				break;
			case 'T':							/* Title for PR */
				clipcopy(control->Title, line+1, LPR_MAX_T);
				break;
			case 'W':							/* Width of output */
				control->width = atoi(line+1);
				break;
			case 'U':							/* Name of file to unlink when done */
				if(line[1] == 'd')				/* <-- filter out spurious lines from */
					control->files_unlinked++;	/*	   lpr's -r switch */
				break;

			case '<':							/* DEC OSF input tray */
				clipcopy(control->LT_inputtray, line+1, LPR_MAX_DEC);
				break;

			case 'K':							/* DEC OSF duplex */
				clipcopy(control->Kduplex, line+1, LPR_MAX_DEC);
				break;

			case 'G':							/* DEC OSF N-Up */
				control->Gnup = atoi(line+1);
				if(control->Gnup > 16)
					control->Gnup = 16;
				if(control->Gnup < 1 )
					control->Gnup = 0;
				break;

			case '>':							/* DEC OSF output tray */
				clipcopy(control->GT_outputtray, line+1, LPR_MAX_DEC);
				break;

			case 'O':							/* DEC OSF orientation */
				clipcopy(control->Oorientation, line+1, LPR_MAX_DEC);
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
					int x;

					if(strncmp(line+1, last_file_name, MAX_NAME_CONSIDER) == 0) /* if same as last name */
						{														/* it means extra copy */
						control->files[control->files_count - 1].copies++;
						DODEBUG_CONTROL_FILE(("(%d copies)", control->files[control->files_count - 1].copies));
						}
					else if( (x=control->files_count) < MAX_FILES_PER_JOB )
						{
						control->files[x].type = *line;
						control->files[x].copies = 1;
						control->files_count++;
						clipcopy(last_file_name, line+1, MAX_NAME_CONSIDER);
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

	/* Do final handshaking. */
	if(fgetc(stdin) == 0)		/* Read zero byte. */
		fputc(0, stdout);		/* If we got it, acknowledge, */
	else						/* otherwise, */
		fputc(1, stdout);		/* deny. */

	fflush(stdout);

	/*
	** If the host name is known and we can look it up sucessfully,
	** construct the mail address for the user at that host.
	**
	** We will not necessarily use the the mail address.
	*/
	if( (control->Host[0] != (char)NULL)
				&& (gethostbyname(control->Host) != (struct hostent*)NULL) )
		{
		if(control->Mailto[0] != (char)NULL )			/* If user has requested mail, */
			strcpy(control->mailaddr, control->Mailto); /* use the name in the request. */
		else											/* Otherwise, use the name */
			strcpy(control->mailaddr, control->Person); /* of the job owner */

		strcat(control->mailaddr, "@");					/* Append "@$host". */
		strcat(control->mailaddr, control->Host);
		}

	/* Build the principal id. */
	sprintf(control->principal, "%s@%s", control->Person, client_dns_name);

	} /* end of receive_control_file() */

/*
** Open the temporary file to hold the data file or files.
** (If we store multiple files in it we will remember the
** starting offset and length of each.)
*/
int open_tmp(void)
	{
	char fname[MAX_PPR_PATH];
	int file;

	ppr_fnamef(fname, "%s/ppr-olprsrv-%ld-XXXXXX", TEMPDIR, (long)getpid());

	if((file = mkstemp(fname)) == -1)
		{
		debug("can't create temp file: %s, errno=%d (%s)", line, errno, gu_strerror(errno));
		return -1;
		}

	DODEBUG_GRITTY(("opened \"%s\", handle=%d", fname, file));

	unlink(fname);						/* we don't need the name any more */

	return file;						/* return pointer to file stream */
	} /* end of open_tmp() */

/*
** Based on the information collected from the control file,
** build an argument list for ppr.
*/
void build_argv_ppr(const char *args[], char *printer, struct CONTROL_FILE *control, int file_index)
	{
	int x;
	char *ppr_type = (char*)NULL;

	DODEBUG_PRINT(("build_argv_ppr()"));

	x = 0;
	args[x++] = PPR_PATH;
	args[x++] = "ppr";			/* name of program being invoked */

	args[x++] = "-d";			/* specify the printer */
	args[x++] = printer;

	/* If we know who we are printing this for, say so. */
	if(control->Person[0] != (char)NULL)
		{
		args[x++] = "-f";
		args[x++] = control->Person;
		}

	/* Pass the principal id to the spooler. */
	args[x++] = "-X";
	args[x++] = control->principal;

	/* If we have a mail address, provide it */
	if(control->mailaddr[0] != (char)NULL)
		{
		args[x++] = "-m";
		args[x++] = "mail";

		args[x++] = "-r";
		args[x++] = control->mailaddr;

		/* If the user didn't ask for mail, arrange to
		   send it only if there is an error.
		   */
		if(control->Mailto[0] == (char)NULL)
			{
			args[x++] = "--responder-options";
			args[x++] = "printed=no";
			}
		}
	/* No idea of the mail address, nothing we can do. */
	else
		{
		args[x++] = "-m";
		args[x++] = "none";
		}

	/*
	** If a job name was supplied for the banner page, pass it
	** to ppr's -C switch.	If it isn't, pass along the file name.
	**
	** (ppr's -C switch specifies the default job title.)
	** This may be overridden in a moment if "lpr -p" was used.
	*/
	if(control->Jobname[0] != (char)NULL)
		{
		args[x++] = "-C";
		args[x++] = control->Jobname;
		}
	else if(control->N_count > file_index)
		{
		args[x++] = "-C";
		args[x++] = control->files[file_index].Name;
		}
	#ifdef LPRSRV_USE_LPR_STYLE_JOB_NAME
	args[x++] = "-R";
	args[x++] = "ignore-title";
	#endif

	/*
	** If we have been told what the name of this file is,
	** pass it on to ppr.
	*/
	if(control->N_count > file_index)
		{
		args[x++] = "--lpq-filename";
		args[x++] = control->files[file_index].Name;
		}

	/*
	** If no job name was specified above and the job is to
	** be passed through pr and no explicit title for the
	** banner page was specified, give it the pr title in
	** the -C switch.
	*/
	#ifndef LPRSRV_USE_LPR_STYLE_JOB_NAME
	if(control->Jobname[0] == (char)NULL && control->files[file_index].type == 'p' && control->Title[0] != (char)NULL)
		{
		args[x++] = "-C";
		args[x++] = control->Title;
		}
	#endif

	/*
	** If the input type was determined, tell PPR about it.
	*/
	switch(control->files[file_index].type)
		{
		/*
		** Plain text file.	 Actually, the user probably has not
		** indicated the file type at all, so we will let PPR
		** auto-detect the type.
		*/
		case 'f':
			break;

		/*
		** Leave control characters.  Supposedly the "f" command
		** above filters out control characters not in the set
		** (HT CR FF LF BS).  Since this request doesn't give
		** any definite information about the file's type,
		** we will let ppr's auto-detect go to work on it.
		*/
		case 'l':
			break;

		/*
		** PostScript.
		*/
		case 'o':
			ppr_type = "postscript";
			break;

		/*
		** Pr format.  PPR has a special type for this and
		** Lpr has the -p switch, but LP probably doesn't have
		** anything for this type.
		*/
		case 'p':
			ppr_type = "pr";
			break;

		/*
		** Fortran carriage control.  In the case of LP, you will
		** probably have to supply your own filter.
		*/
		case 'r':
			ppr_type = "fortran";
			break;

		/*
		** CIF file.  In the case of LP, you will probably have
		** to supply your own filter.
		*/
		case 'c':
			ppr_type = "cif";
			break;

		/*
		** Plot file.  System V generally has filters for this.
		*/
		case 'g':
			ppr_type = "plot";
			break;

		/*
		** Sun raster file.	 (The DEC man page says "devices like
		** Benson Varian".)	 PPR does not have a filter for this
		** format.
		*/
		case 'v':
			ppr_type = "sunras";
			break;

		/*
		** Ditroff output.
		*/
		case 'n':
			ppr_type = "troff";
			break;

		/*
		** TeX DVI file.
		*/
		case 'd':
			ppr_type = "dvi";
			break;

		/*
		** Old Troff (CAT/4).
		*/
		case 't':
			ppr_type = "cat4";
			break;

		/*
		** No filtering.  (From DEC Lpr man page.)
		** It is difficult to know how to handle this.
		*/
		case 'x':
			break;
		}

	if(ppr_type != (char*)NULL)
		{
		args[x++] = "-T";
		args[x++] = ppr_type;
		}

	/* Pass multiple copies requests to the spooler. */
	if(control->files[file_index].copies != 1 && control->files[file_index].copies < 1000)
		{
		static char copies_str[4];
		args[x++] = "-n";
		sprintf(copies_str, "%d", control->files[file_index].copies);
		args[x++] = copies_str;
		}

	/*
	** If the user has asked for suppression of the banner page,
	** try to suppress it here.
	*/
	if(! control->print_banner)
		{
		args[x++] = "-b";
		args[x++] = "no";
		}

	/* Possibly include the width: */
	if(control->width && control->width < 1000)
		{
		static char width_str[10];

		args[x++] = "-o";
		sprintf(width_str, "width=%d", control->width);
		args[x++] = width_str;
		}

	/* DEC OSF Input tray: */
	if( control->LT_inputtray[0] )
		{
		static char temp_inputtray[sizeof("*InputSlot ") + LPR_MAX_DEC + 1];

		args[x++] = "-F";
		control->LT_inputtray[0] = toupper(control->LT_inputtray[0]);
		sprintf(temp_inputtray, "*InputSlot %s", control->LT_inputtray);
		args[x++] = temp_inputtray;
		}

	/* DEC OSF Output tray: */
	if( control->GT_outputtray[0] )
		{
		static char temp_outputtray[sizeof("OutputBin ") + LPR_MAX_DEC + 1];

		args[x++] = "-F";
		control->GT_outputtray[0] = toupper(control->GT_outputtray[0]);
		sprintf(temp_outputtray, "*OutputBin %s", control->GT_outputtray);
		args[x++] = temp_outputtray;
		}

	/* DEC OSF Orientation.	 Many filters will ignore this. */
	if(control->Oorientation[0])
		{
		char *ptr;
		static char temp_orientation[sizeof("orientation=") + LPR_MAX_DEC + 1];

		args[x++] = "-o";
		for(ptr=control->Oorientation; *ptr; ptr++) *ptr = tolower(*ptr);
		sprintf(temp_orientation, "orientation=%s", control->Oorientation);
		args[x++] = temp_orientation;
		}

	/*
	** DEC OSF Duplex settings.	 Not all filters implement
	** all of these modes.	We insert a filter option so that
	** the filter may know how to format the job.  We insert a
	** -F switch, in the first three instances, so that the
	** desired mode will be selected even if the filter does
	** not implement the duplex= option; in the last three
	** instances, in order to override the duplex mode selected
	** by the filter.  This is because the last three options
	** call for margins and gutters appropriate for duplex or simplex
	** but actual printing in the opposite mode.
	*/
	if(control->Kduplex[0])
		{
		/* Simplex */
		if(strcmp(control->Kduplex, "one") == 0)
			{
			args[x++] = "-o";
			args[x++] = "duplex=none";
			args[x++] = "-F";
			args[x++] = "*Duplex None";
			}
		/* Duplex */
		if(strcmp(control->Kduplex, "two") == 0)
			{
			args[x++] = "-o";
			args[x++] = "duplex=notumble";
			args[x++] = "-F";
			args[x++] = "*Duplex DuplexNoTumble";
			}
		/* Duplex Tumble */
		if(strcmp(control->Kduplex, "tumble") == 0)
			{
			args[x++] = "-o";
			args[x++] = "duplex=tumble";
			args[x++] = "-F";
			args[x++] = "*Duplex DuplexTumble";
			}
		/* Format for duplex, force simplex */
		if(strcmp(control->Kduplex, "one_sided_duplex") == 0)
			{
			args[x++] = "-o";
			args[x++] = "duplex=notumble";
			args[x++] = "-F";
			args[x++] = "*Duplex None";
			}
		/* Format for duplex tumble, force simplex */
		if(strcmp(control->Kduplex, "one_sided_tumble") == 0)
			{
			args[x++] = "-o";
			args[x++] = "duplex=tumble";
			args[x++] = "-F";
			args[x++] = "*Duplex None";
			}
		/* Format for simplex, force duplex */
		if(strcmp(control->Kduplex, "two_sided_simplex") == 0)
			{
			args[x++] = "-o";
			args[x++] = "duplex=none";
			args[x++] = "-F";
			args[x++] = "*Duplex Duplex";
			}
		}

	/*
	** DEC OSF N-Up:
	**
	** This _MUST_ come after the duplex option code.  This is
	** because if N-Up is invoked we want to override the
	** duplex= filter option so that the filters will not insert
	** gutters.	 (Gutters would look silly on N-Up printed pages.)
	*/
	if(control->Gnup > 0)
		{
		static char temp_nup[4];

		args[x++] = "-N";
		sprintf(temp_nup, "%d", control->Gnup);
		args[x++] = temp_nup;
		args[x++] = "-o";
		args[x++] = "duplex=undef";
		}

	/*
	** Report errors with responder and not stderr:
	*/
	args[x++] = "-e";
	args[x++] = "responder";

	/*
	** Append warning messages about irregularities in the input
	** file to the print job's log file and print them on banner
	** page if there is one.
	*/
	args[x++] = "-w";
	args[x++] = "log";

	/* Include the destination's switchset. */
	args[x++] = "-I";

	args[x] = (char*)NULL;		/* terminate the list */
	} /* end of build_argv_ppr() */

/*
** Based on the information collected from the control file,
** build an argument list for lpr.
*/
void build_argv_lpr(const char *args[], char *printer, struct CONTROL_FILE *control, int file_index)
	{
	int x;

	DODEBUG_PRINT(("build_argv_lpr()"));

	x = 0;
	args[x++] = uprint_path_lpr();
	args[x++] = "lpr";

	args[x++] = "-P";			/* specify the printer */
	args[x++] = printer;

	/*
	** Pass on the user name
	** information, even though LPR may not think we
	** have sufficent priviledge to be allowed to
	** declare ourself to be representing somebody else./
	** (See the lpr(1) man page.)
	*/
	if(control->Person[0] != (char)NULL)
		{
		args[x++] = "-U";
		args[x++] = control->Person;
		}

	/* If the number of copies is not 1, pass it on. */
	if(control->files[file_index].copies > 1 && control->files[file_index].copies < 999)
		{
		static char copies_str[4];

		args[x++] = "-#";
		sprintf(copies_str, "%d", control->files[file_index].copies);
		args[x++] = copies_str;
		}

	/*
	** If we know the jobname, try to pass
	** it on to LPR.
	*/
	if(control->Jobname[0] != (char)NULL)
		{
		args[x++] = "-J";
		args[x++] = control->Jobname;
		}

	/*
	** Pass the class identifier string on to LPR.
	*/
	if(control->Class[0] != (char)NULL)
		{
		args[x++] = "-C";
		args[x++] = control->Class;
		}

	/* Pass on the title string. */
	if(control->Title[0] != (char)NULL)
		{
		args[x++] = "-T";
		args[x++] = control->Title;
		}

	/*
	** If the user has asked for suppression of the banner page,
	** try to suppress it here.
	*/
	if(! control->print_banner)
		args[x++] = "-h";

	/* If the type was determined, tell LPR about it */
	switch(control->files[file_index].type)
		{
		/*
		** Plain text file.	 Actually, the user probably has not
		** indicated the file type at all.
		*/
		case 'f':
			break;

		/*
		** Leave control characters.  Supposedly the "f" command
		** above filters out control characters not in the set
		** (HT CR FF LF BS).
		*/
		case 'l':
			args[x++] = "-l";
			break;

		/*
		** PostScript.	It seems that Lpr does not have
		** a switch for this type.
		*/
		case 'o':
			break;

		/*
		** Pass thru pr:
		*/
		case 'p':
			args[x++] = "-p";
			break;

		/*
		** Fortran carriage control:
		*/
		case 'r':
			args[x++] = "-f";
			break;

		/*
		** CIF file:
		*/
		case 'c':
			args[x++] = "-c";
			break;

		/*
		** Plot file:
		*/
		case 'g':
			args[x++] = "-g";
			break;

		/*
		** Sun raster file.	 (The DEC man page says "devices like
		** Benson Varian".)
		*/
		case 'v':
			args[x++] = "-v";
			break;

		/*
		** Ditroff output:
		*/
		case 'n':
			args[x++] = "-n";
			break;

		/*
		** TeX DVI file:
		*/
		case 'd':
			args[x++] = "-d";
			break;

		/*
		** Old Troff (CAT/4):
		*/
		case 't':
			args[x++] = "-t";
			break;

		/*
		** No filtering.  (From DEC Lpr man page.)
		** It is difficult to know how to handle this since
		** lpr on this computer may not have a "-x" switch.
		*/
		case 'x':
			args[x++] = "-x";
			break;
		} /* end of file type switch */

	/* Possibly include the width. */
	if(control->width && control->width < 1000)
		{
		static char width_str[10];		/* space to format width to pass to spooler ("width=9999") */

		args[x++]="-w";
		sprintf(width_str,"%d",control->width);
		args[x++] = width_str;
		}

	/* Possibly include an indent. */
	if(control->indent > 0 && control->indent < 1000)
		{
		static char indent_str[10];

		args[x++] = "-i";
		sprintf(indent_str, "%d",control->indent);
		args[x++] = indent_str;
		}

	args[x] = (char*)NULL;		/* terminate the list */
	} /* end of build_argv_lpr() */

/*
** Based on the information collected from the control file,
** build an argument list for lp.
*/
void build_argv_lp(const char *args[], char *printer, struct CONTROL_FILE *control, int file_index)
	{
	int x;
	char *lp_type = (char*)NULL;

	DODEBUG_PRINT(("build_argv_lp()"));

	x = 0;
	args[x++] = uprint_path_lp();
	args[x++] = "lp";

	/* Specify the printer desired: */
	args[x++] = "-d";
	args[x++] = printer;

	/*
	** If we are sending to LP, we will want to
	** suppress the message which tells us what
	** the queue id is.
	*/
	args[x++] = "-s";

	/* If the number of copies is not 1, pass it on. */
	if(control->files[file_index].copies > 1 && control->files[file_index].copies < 999)
		{
		static char copies_str[4];

		args[x++] = "-n";
		sprintf(copies_str, "%d", control->files[file_index].copies);
		args[x++] = copies_str;
		}

	/*
	** If we know the jobname, try to pass
	** it on to LP.
	*/
	if(control->Jobname[0] != (char)NULL)
		{
		args[x++] = "-t";
		args[x++] = control->Jobname;
		}

	/*
	** If the user has asked for suppression of the banner page,
	** try to suppress it here.
	*/
	if(! control->print_banner)
		{
		args[x++] = "-o";
		args[x++] = "nobanner";
		}

	switch(control->files[file_index].type)
		{
		/*
		** Plain text file.	 Actually, the user probably has not
		** indicated the file type at all.
		*/
		case 'f':
			lp_type = "simple";
			break;

		/*
		** Leave control characters.  Supposedly the "f" command
		** above filters out control characters not in the set
		** (HT CR FF LF BS).
		*/
		case 'l':
			break;

		/*
		** PostScript:
		*/
		case 'o':
			lp_type = "postscript";
			break;

		/*
		** Pr format.  PPR has a special type for this and
		** Lpr has the -p switch, but LP probably doesn't have
		** anything for this type.
		*/
		case 'p':
			break;

		/*
		** Fortran carriage control.  In the case of LP, you will
		** probably have to supply your own filter.
		*/
		case 'r':
			lp_type = "fortran";
			break;

		/*
		** CIF file.  In the case of LP, you will probably have
		** to supply your own filter.
		*/
		case 'c':
			lp_type = "cif";
			break;

		/*
		** Plot file.  System V generally has filters for this.
		*/
		case 'g':
			lp_type = "plot";
			break;

		/*
		** Sun raster file.	 (The DEC man page says "devices like
		** Benson Varian".)
		*/
		case 'v':
			lp_type = "sunras";

		/*
		** Ditroff output.
		*/
		case 'n':
			lp_type = "troff";
			break;

		/*
		** TeX DVI file:
		*/
		case 'd':
			lp_type = "dvi";
			break;

		/*
		** Old Troff (CAT/4).
		*/
		case 't':
			lp_type = "otroff";
			break;

		/*
		** No filtering.  (From DEC Lpr man page.)
		*/
		case 'x':
			args[x++] = "-r";
			break;
		}

	if(lp_type != (char*)NULL)
		{
		args[x++] = "-T";
		args[x++] = lp_type;
		}

	args[x] = (char*)NULL;		/* terminate the list */
	} /* end of build_argv_lp() */

/*
** Send an already collected datafile to ppr, lp, or lpr.
*/
int dispatch_file(int tempfile, char *printer, struct CONTROL_FILE *control, struct DATA_FILE *data, int file_index)
	{
	const char *args[100];				/* List of arguments */
	pid_t pid;							/* process id of PPR or LP */
	size_t file_bytes_remaining;		/* amount of file left */
	int fds[2];							/* file descriptors of pipe */

	DODEBUG_PRINT(("dispatch_file(): printer=%s, file_index=%d, start=%ld, len=%ld", printer, file_index, data[file_index].start, data[file_index].length));

	/* Go to the right place in the temporary file: */
	if(lseek(tempfile, data[file_index].start, SEEK_SET) == -1)
		fatal(1, "dispatch_file(): lseek() failed");

	/* Determine how long it is: */
	file_bytes_remaining = data[file_index].length;

	/* Build a command line appropriate for the spooler: */
	if(printdest_claim_ppr(printer))
		build_argv_ppr(args, printer, control, file_index);

	else if(printdest_claim_bsd(printer))
		build_argv_lpr(args, printer, control, file_index);

	else if(printdest_claim_sysv(printer))
		build_argv_lp(args, printer, control, file_index);

	else
		fatal(1, "Queue \"%s\" does not exist on server!", printer);

	/* This code is used to test for problems in the
	   build_argv*() functions called above. */
	#ifdef DEBUG_PRINT
	{
	int x;
	FILE *logfile;

	if( (logfile=fopen(LPRSRV_LOGFILE, "a")) != NULL )
		{
		for(x=0; args[x]; x++)
			{
			fputs(args[x], logfile);
			fputs(" ", logfile);
			}
		fputc('\n', logfile);
		fclose(logfile);
		}
	}
	#endif

	/* Open a pipe which will be used to connect us to the child: */
	if(pipe(fds) == -1)
		fatal(1, "dispatch_file(): pipe() failed, errno=%d (%s)", errno, gu_strerror(errno) );

	/* Keep trying until we can fork() a child. */
	while( (pid=fork()) == -1 )
		{
		debug("dispatch_file(): fork() failed, retry in 60 seconds");
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
				if( (readlen = read(tempfile, buffer, file_bytes_remaining > sizeof(buffer) ? sizeof(buffer) : file_bytes_remaining)) == -1 )
					fatal(1, "dispatch_file(): read() failed, errno=%d (%s)", errno, gu_strerror(errno) );

				if(readlen == 0)
					fatal(1, "dispatch_file(): defective temp file?");

				written = 0;
				file_bytes_remaining -= readlen;
				}

			if( (thiswrite = write(fds[1], buffer+written, readlen-written)) == -1 )
				fatal(1, "dispatch_file(): write() failed, errno=%d (%s)", errno, gu_strerror(errno) );
			written += thiswrite;
			} while(readlen > written || file_bytes_remaining);

		/* If we don't do this ppr/lpr/lp will wait forever: */
		close(fds[1]);

		/* Wait for PPR or LP/LPR to terminate. */
		DODEBUG_PRINT(("Waiting for %s to exit...", args[0]));
		wait(&wstat);

		if(WCOREDUMP(wstat))
			{
			debug("%s dumped core", args[0]);
			}
		else if(WIFEXITED(wstat))
			{
			switch(WEXITSTATUS(wstat))
				{
				case 0:
					DODEBUG_PRINT(("%s ran normally", args[0]));
					break;
				case 254:
					debug("Child can't open log file");
					break;
				case 255:
					debug("Exec() of spooler program failed");
					break;
				default:
					debug("%s exited with code %d", args[0], WEXITSTATUS(wstat));
					break;
				}
			}
		else
			{
			debug("%s terminated by signal %d ***", args[0], WTERMSIG(wstat));
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
		if( (log=open(LPRSRV_LOGFILE, O_WRONLY | O_APPEND, 1)) == -1 )
			_exit(254);

		/* Connect stdout and stderr to the log file. */
		dup2(log, 1);
		dup2(log, 2);
		close(log);				/* We don't need this descriptor any more. */

		execv(args[0], (char **)&args[1]);

		_exit(255);				/* exit here if exec failed */
		}

	return 1;
	} /* end of dispatch_file() */

/*
** Implement the ^B command
** (The ^B command receives a print job.)
*/
void take_job(char *command)
	{
	int tempfile=-1;					/* file handle of temp file */
	char printer[32];
	int control_file_received = FALSE;	/* TRUE if we have the control file */
	struct CONTROL_FILE control;
	struct DATA_FILE data_files[MAX_FILES_PER_JOB];
	int files_on_hand = 0;

	/* Remember the name of the printer. */
	clipcopy(printer, command+1, sizeof(printer) );

	/* Acknowledge ^B command */
	fputc(0, stdout);
	fflush(stdout);

	/* Clear the structure which describes the control file */
	clear_control(&control);

	/* subcommand loop */
	while(lprsrv_getline())				/* Get a line with a subcommand on it. */
		{
		switch(line[0])
			{
			case 1:						/* Abort job */
				DODEBUG_PRINT(("abort job"));

				if(tempfile != -1)
					close(tempfile);

				return;					/* take_job() is done */

			case 2:						/* control file */
				DODEBUG_PRINT(("control file: %s", &line[1]));

				receive_control_file(&line[1], &control);
				control_file_received = TRUE;

				break;

			case 3:						/* data file */
				DODEBUG_PRINT(("data file: %s", &line[1]));

				/* If the tempfile already open or we can open it, */
				if(tempfile != -1 || (tempfile=open_tmp()) != -1)
					{
					data_files[files_on_hand].start = lseek(tempfile, (off_t)0, SEEK_CUR);
					data_files[files_on_hand].length = receive_data_file(&line[1], tempfile);
					if((files_on_hand + 1) < MAX_FILES_PER_JOB)
						files_on_hand++;
					}

				/* If couldn't open, say there is no room.
				   Doing this will cause the remote end
				   to try again later.	I don't know if this
				   is a good idea or not. */
				else
					{
					fputc(2, stdout);
					fflush(stdout);
					}

				break;

			default:
				debug("unreconized subcommand: %c",line[0]);

				/* I don't remember why I chose an error code of 2 here. */
				fputc(2, stdout);
				fflush(stdout);

				break;
			} /* end of switch */

		/*
		** If we have a control file and as many data files
		** as the control file says we should have, go print them.
		*/
		if(control_file_received && files_on_hand >= control.files_unlinked)
			{
			int x;

			DODEBUG_PRINT(("dispatching %d job(s)", files_on_hand));

			/* Print all the jobs */
			for(x=0; x < files_on_hand; x++)
				dispatch_file(tempfile, printer, &control, data_files, x);

			/* get ready for next job */
			close(tempfile);
			tempfile = -1;
			files_on_hand = 0;
			clear_control(&control);
			control_file_received = FALSE;
			}

		} /* end of line reading loop */
	} /* end of take_job() */

/*=================================================================
** List the files in the queue
** We get the command character so we can know if it
** should be a long or short listing.
=================================================================*/
void show_jobs(char *command)
	{
	int format;
	char *queue, *list;
	int i, list_length, item_length;
	char *item_ptr;
	#define args_SIZE 25
	const char *args[args_SIZE];
	int x;

	format = *command;			/* long or short */
	queue = command + 1;		/* name of queue */
	list = queue;
	list += strcspn(list, " ");
	*(list++) = (char)NULL;		/* terminate name of queue */
	list += strspn(list, " ");

	x = 0;
	if(printdest_claim_ppr(queue))
		{
		args[x++] = PPOP_PATH;
		args[x++] = "ppop";

		if(arrest_interesting_time != (char*)NULL)
			{
			args[x++] = "-A";
			args[x++] = arrest_interesting_time;
			}

		/* short or long format */
		if(format == 3) args[x++] = "lpq";
		else args[x++] = "nhlist";

		args[x++] = queue;
		}

	else if(printdest_claim_bsd(queue))
		{
		args[x++] = uprint_path_lpq();
		args[x++] = "lpq";
		args[x++] = "-P";
		if(format != 4)					/* long format */
			args[x++] = "-l";
		args[x++] = queue;
		}

	else if(printdest_claim_sysv(queue))
		{
		args[x++] = uprint_path_lpstat();
		args[x++] = "lpstat";
		args[x++] = "-o";
		args[x++] = queue;
		}

	else
		{
		fatal(1, "Queue \"%s\" does not exist on server!", queue);
		}

	/* Copy the additional argument to the argumetn list. */
	for(i=0, list_length=strlen(list); i < list_length; i += item_length, i++)
		{
		item_ptr = &list[i];
		item_length = strcspn(item_ptr, " ");
		item_ptr[item_length] = (char)NULL;

		if(x < (args_SIZE-1)) args[x++] = item_ptr;
		}

	args[x++] = (char*)NULL;

	/* We don't need to do anything else, so
	   we can replace this server with ppop,
	   lpq, or lpstat. */
	execv(args[0], (char **)&args[1]);

	/* If execv() fails it returns! */
	fatal(1, "show_jobs(): exec() failed");
	} /* end of show_jobs() */

/*====================================================================
** Remove jobs
====================================================================*/

/*
** This function is called from remove_jobs() below.  It removes
** jobs from a PPR print queue.
**
** This function should implement full lpd node based permisions.  Here
** is how I understand those permissions:
**
** 1) An ordinary  user may delete a specific job only if he sent it and is
**	  deleting it from the node he sent it from.  (Notice that this does not
**	  imply that a user on Node A may use ppr to submit a job and then use
**	  lprm pointing at lprsrv on Node A to delete it.  All jobs received
**	  thru lprsrv are considered to be in some sense remote jobs, even
**	  if they are `loopback' jobs.	A job submitted thru ppr is a local job.
**	  Local jobs may only be deleted thru lprsrv under limited circumstances
**	  described in the following rules.)
**
** 2) Root may delete any job by specifying its id number provided that the
**	  job came from his node except when his node is in the list supplied
**	  with -S, in which case he may delete any job at all including jobs
**	  that were not submitted thru lpr.
**
** 3) If an ordinary user does not specify which jobs to delete, any
**	  jobs which he submitted from the deleting node which are being
**	  printed will be deleted.
**
** 4) If root on a node which is not in the -S list does not specify
**	  which jobs should be deleted, any job submitted from his node
**	  thru lprsrv which is currently being printed is deleted.
**
** 5) If root on a node which is in the -S list does not specify
**	  which jobs to delete, any job which is currently being printed
**	  is deleted regardless of where it came from and who submitted it.
**
** 6) If a users tries to delete all of his own jobs, all jobs which he
**	  submitted thru lprsrv from the node from which the delete request came
**	  will be deleted.
**
** 7) If a user wants to delete all of another's jobs, he must be root
**	  and only jobs with the specified user name that came from his node
**	  thru lprsrv will be deleted, unless the node is in the -S list in
**	  which case all jobs from that user which were submitted thru lprsrv
**	  are deleted.
**
** 8) If root on a node which is not in the -S list tries to delete all jobs,
**	  all jobs submitted thru lprsrv from that node are deleted.
**
** 9) If root on a node which is in the -S list tries to delete all jobs,
**	  all jobs, no matter how they were submitted are deleted.
*/
static int remove_jobs_ppop_cancel(const char *queue, const char *agent, char *list)
	{
	int result_code = 0;
	char proxy_for[LPR_MAX_P+1+LPR_MAX_H+1];
	char *item_ptr;
	int x, list_length, item_length;
	char job_name[MAX_DESTNAME+4+1];
	const char *args[6];
	int i;
	int super_root = FALSE;

	DODEBUG_LPRM(("remove_jobs_ppr(queue=\"%s\", agent=\"%s\", list=\"%s\")", queue, agent, list));

	if(strcmp(agent, "root") == 0 || strcmp(agent, "-all") == 0)
		super_root = is_super_root(client_dns_name);

	if(strlen(queue) > MAX_DESTNAME)
		{
		printf("The print queue name \"%s\" is too long for PPR.\n", queue);
		return 1;
		}

	/*
	** Build a string which represents the user who
	** is requesting the deletions.
	*/
	sprintf(proxy_for, "%s@%s", strcmp(agent, "root") == 0 ? "*": agent, client_dns_name);

	args[0] = "ppop";

	/* Process the list of jobs to be deleted, one job at a time. */
	for(x=0, list_length=strlen(list); x < list_length; x += item_length, x++)
		{
		item_ptr = &list[x];
		item_length = strcspn(item_ptr, " ");
		item_ptr[item_length] = (char)NULL;

		i = 1;			/* argv[] index */

		/* If a job id, */
		if(strspn(item_ptr, "0123456789") == item_length)
			{
			sprintf(job_name, "%s-%s", queue, item_ptr);
			DODEBUG_LPRM(("remove_jobs_ppr(): proxy for \"%s\", removing \"%s\"", proxy_for, job_name));
			if(! super_root)
				{
				args[i++] = "-X";
				args[i++] = proxy_for;
				}
			args[i++] = strcmp(agent, "root") ? "scancel" : "cancel";
			args[i++] = job_name;
			}

		/* Otherwise, it must be a user name: */
		else
			{
			char special_proxy_for[LPR_MAX_P+1+LPR_MAX_H+1];

			/* If not deleting own jobs and not root, */
			if(strcmp(agent, item_ptr) && strcmp(agent, "root"))
				{
				printf("You may not delete jobs belonging to \"%.*s@%s\" because\n"
						"you are not \"root@%s\".\n", item_length, item_ptr, client_dns_name, client_dns_name);
				result_code = 1;
				continue;
				}

			if(item_length > LPR_MAX_P)
				{
				printf("User name \"%s\" is too long.\n", item_ptr);
				result_code = 1;
				continue;
				}

			sprintf(special_proxy_for, "%s@%s", item_ptr, super_root ? "*" : client_dns_name);

			DODEBUG_LPRM(("remove_jobs_ppr(): removing all \"%s\" jobs for \"%s\"", special_proxy_for, queue));
			args[i++] = "-X";
			args[i++] = special_proxy_for;
			args[i++] = strcmp(agent, "root") ? "scancel" : "cancel";
			args[i++] = (char*)queue;
			}

		args[i] = (char*)NULL;

		if(run(PPOP_PATH, args) == -1)
			result_code = 1;
		}

	/* If no files or users were specified, */
	if(list_length == 0)
		{
		i = 1;

		/* If the agent is "-all", */
		if(strcmp(agent, "-all") == 0)
			{
			/* If super_root, purge the queue, */
			if(super_root)
				{
				args[i++] = "purge";
				args[i++] = queue;
				}
			/* Not super_root, delete all from node, */
			else
				{
				char all_mynode[2 + LPR_MAX_H + 1];
				sprintf(all_mynode, "*@%s", client_dns_name);
				args[i++] = "-X";
				args[i++] = all_mynode;
				args[i++] = "cancel";
				args[i++] = queue;
				}
			}

		/* Agent not "-all", delete active job, */
		else
			{
			if(super_root)
				{
				DODEBUG_LPRM(("Removing active job on %s", queue));
				args[i++] = "cancel-active";
				args[i++] = queue;
				}
			else
				{
				DODEBUG_LPRM(("Removing active job for \"%s\" on %s", proxy_for, queue));
				args[i++] = "-X";
				args[i++] = proxy_for;
				args[i++] = strcmp(agent, "root") ? "scancel-my-active" : "cancel-my-active";
				args[i++] = queue;
				}
			}

		args[i] = (char*)NULL;

		if(run(PPOP_PATH, args) == -1)
			result_code = 1;
		}

	return result_code;
	} /* end of remove_jobs_ppop_cancel() */

/*
** Function to remove jobs from a System V LP spooling system.
** This is new and needs testing.
**
** It does not implement node based security as
** remove_jobs_ppop_cancel() does.
*/
static int remove_jobs_cancel(const char *queue, const char *agent, char *list)
	{
	int retval = 0;
	int list_length, x, item_length;
	char *item_ptr;
	const char *args[10];
	char request_id[30+1+8+1];			/* generous but arbitrary */

	DODEBUG_LPRM(("remove_jobs_cancel(queue=\"%s\", agent=\"%s\", list=\"%s\")", queue, agent, list));

	if(strlen(queue) > 30)
		{
		printf("The print queue name \"%s\" is too long.\n", queue);
		return 1;
		}

	args[0] = "cancel";

	for(x=0, list_length=strlen(list); x < list_length; x += item_length, x++)
		{
		item_ptr = &list[x];
		item_length = strcspn(item_ptr, " ");
		item_ptr[item_length] = (char)NULL;

		if(strspn(item_ptr, "0123456789") == item_length)
			{
			if(item_length > 8)
				{
				printf("The queue id \"%.*s\" is too long.\n", item_length, item_ptr);
				retval = 1;
				continue;
				}
			sprintf(request_id, "%s-%s", queue, item_ptr);
			args[1] = request_id;
			args[2] = (char*)NULL;
			}
		else
			{
			if(strcmp(item_ptr, agent) && strcmp(agent, "root"))
				{
				printf("You cannot delete jobs belonging to \"%s\" because\n"
						"you are not root.\n", item_ptr);
				retval = 1;
				continue;
				}
			args[1] = "-u";
			args[2] = item_ptr;
			args[3] = queue;
			args[4] = (char*)NULL;
			}

		if(run(uprint_path_cancel(), args) == -1)
			retval = 1;
		}

	if(list_length == 0)		/* cancel current request */
		{
		args[1] = queue;
		args[2] = (char*)NULL;
		if(run(uprint_path_cancel(), args) == -1)
			retval = 1;
		}

	return retval;
	} /* end of remove_jobs_cancel() */

/*
** Function to remove the jobs from a BSD lpr spooling system.
** This is new and needs testing.
**
** This does not implement the complex node dependent permissions that
** the real lpd does and remove_jobs_ppr() do.
**
*/
static int remove_jobs_lprm(const char *queue, const char *agent, char *list)
	{
	int retval = 0;
	int list_length, x, item_length;
	char *item_ptr;
	int y;
	const char *args[101];

	DODEBUG_LPRM(("remove_jobs_lprm(queue=\"%s\", agent=\"%s\", list=\"%s\")", queue, agent, list));

	y = 0;
	args[y++] = "lprm";
	args[y++] = "-P";
	args[y++] = queue;

	/*
	** Separate the list into words and add each word to the
	** argument list we will pass to lprm.
	*/
	for(x=0, list_length=strlen(list); x < list_length; x += item_length, x++)
		{
		item_ptr = &list[x];
		item_length = strcspn(item_ptr, " ");

		if(y < 100)				/* only use it if there is room */
			{
			args[y++] = item_ptr;
			item_ptr[item_length] = (char)NULL;
			}
		}

	args[y++] = (char*)NULL;

	if(run(uprint_path_lprm(), args) == -1)
		retval = 1;

	return retval;
	} /* end of remove_jobs_lprm() */

/*
** Handler for the ^E command.	It does some preliminary parsing
** and then passes the work on to a spooler specific function
** defined above.
*/
void remove_jobs(char *command)
	{
	char *queue;				/* queue to delete the jobs from */
	char *agent;				/* user requesting the deletion */
	char *list;					/* list of ids or user's to delete */
	int retcode;

	command++;
	queue = command;					/* first is queue to delete from */
	command += strcspn(queue, " ");
	*command = (char)NULL;

	command++;							/* second is agent making request */
	agent = command;
	command += strcspn(command, " ");
	*command = (char)NULL;

	command++;
	list = command;
	command[strcspn(command, "\n")] = (char)NULL;

	DODEBUG_LPRM(("remove_jobs(): queue=\"%s\", agent=\"%s\", list=\"%s\"", queue, agent, list));

	if(strlen(agent) > LPR_MAX_P)
		{
		DODEBUG_LPRM(("The user name \"%s\" is too long", agent));
		printf("The user name \"%s\" is too long.\n", agent);
		/* fputc(1, stdout); */
		fflush(stdout);
		return;
		}

	/*
	** Determine which spooler the queue belongs to
	** and call the appropriate action function.
	*/
	if(printdest_claim_ppr(queue))
		retcode = remove_jobs_ppop_cancel(queue, agent, list);

	else if(printdest_claim_bsd(queue))
		retcode = remove_jobs_lprm(queue, agent, list);

	else if(printdest_claim_sysv(queue))
		retcode = remove_jobs_cancel(queue, agent, list);

	else
		{
		printf("Unknown print queue: %s\n", queue);
		retcode = 1;
		}

	DODEBUG_LPRM(("remove_jobs(): result code is %d", retcode));
	/* fputc(retcode, stdout);
	fflush(stdout); */
	} /* end of remove_jobs() */

/*===========================================================
** These two functions, reapchild() bind_server(),
** and be_server() are used in standalone mode.
===========================================================*/

/*
** This sets the effective uid and gid to "ppr" and "ppr"
** respectively but does not change the real ids.
**
** After it looks up the ids for user "ppr" and the group "ppr"
** it saves them in the variables pointed to by its arguments.
*/
void half_demote_self(uid_t *uid_ppr, gid_t *gid_ppop)
	{
	struct passwd *user_ppr;
	struct group *group_ppop;

	if( (user_ppr = getpwnam(USER_PPR)) == (struct passwd *)NULL )
		fatal(1, "getpwnam(\""USER_PPR"\") failed");

	if( (group_ppop = getgrnam(GROUP_PPR)) == (struct group *)NULL )
		fatal(1, "getgrnam(\""GROUP_PPR"\") failed");

	*uid_ppr = user_ppr->pw_uid;
	*gid_ppop = group_ppop->gr_gid;

	if(setegid(*gid_ppop) == -1)
		fatal(1, "half_demote_self(): setegid(%ld) failed", (long)*gid_ppop);
	if(seteuid(*uid_ppr) == -1)
		fatal(1, "half_demote_self(): seteuid(%ld) failed", (long)*uid_ppr);
	}

/*
** This sets all user and all group ids to "ppr" and "ppr"
** respectively.
*/
void fully_demote_self(uid_t uid_ppr, gid_t gid_ppr)
	{
	if(setuid(0) == -1)
		fatal(1, "fully_demote_self(): setuid(0) failed");

	if(setgid(0) == -1)
		fatal(1, "fully_demote_self(): setgid(0) failed");

	if(setgid(gid_ppr) == -1)
		fatal(1, "fully_demote_self(): setgid(%ld) failed", (long)gid_ppr);

	if(setuid(uid_ppr) == -1)
		fatal(1, "fully_demote_self(): setuid(%ld) failed", (long)uid_ppr);

	if(setregid(gid_ppr, gid_ppr) == -1)
		fatal(1, "fully_demote_self(): setregid(%ld, %ld) failed", (long)gid_ppr, (long)gid_ppr);

	if(setreuid(uid_ppr, uid_ppr) == -1)
		fatal(1, "fully_demote_self(): setreuid(%ld, %ld) failed", (long)uid_ppr, (long)uid_ppr);

	if(setuid(0) != -1)
		fatal(1, "fully_demote_self(): setuid(0) did not fail", (long)uid_ppr, (long)uid_ppr);
	}

/*
** Create the lock file which exists mainly so that we
** can be killed:
*/
void create_lock_file(void)
	{
	int lockfilefd;
	char temp[10];

	if( (lockfilefd=open(LPRSRV_LOCKFILE, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR)) == -1 )
		fatal(1, "lprsrv: can't open \"%s\", errno=%d (%s)", LPRSRV_LOCKFILE, errno, gu_strerror(errno));

	if(gu_lock_exclusive(lockfilefd, FALSE))
		fatal(1, "lprsrv: already running standalone");

	sprintf(temp, "%ld\n", (long)getpid());
	write(lockfilefd, temp, strlen(temp));
	}

/*
** Create the server well known port and return a
** file descriptor for it.
*/
int bind_server(int server_port, uid_t ppr_uid)
	{
	int sockfd;
	struct sockaddr_in serv_addr;
	int bind_result;

	if( (sockfd=socket(AF_INET, SOCK_STREAM,0)) == -1 )
		fatal(1, "be_server(): socket() failed, errno=%d (%s)", errno, gu_strerror(errno));

	/* We will accept from any IP address and will listen on server_port. */
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(server_port);

	/* Try to avoid being locked out when restarting daemon: */
	{
	int one = 1;
	if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&one, sizeof(one)) == -1)
		fatal(1, "be_server(): setsockopt() failed, errno=%d (%s)", errno, gu_strerror(errno));
	}

	/* Become root mementarily in order to bind the port. */
	seteuid(0);
	bind_result = bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
	seteuid(ppr_uid);

	if(bind_result == -1)
		fatal(1, "be_server(): bind() failed, errno=%d (%s)", errno, gu_strerror(errno));

	/* Set the backlog queue length. */
	if(listen(sockfd, 5) == -1)
		fatal(1, "be_server(): listen() failed, errno=%d (%s)", errno, gu_strerror(errno));

	return sockfd;
	} /* end of bind_server() */

/*
** This function is called in the daemon whenever one of the
** children launched to service a connexion exits.
*/
void reapchild(int sig)
	{
	int pid, stat;

	while( (pid=waitpid((pid_t)-1, &stat, WNOHANG)) > (pid_t)0 )
		{
		DODEBUG_STANDALONE(("child %ld terminated", (long)pid));
		}
	} /* end of reapchild() */

/*
** This function is called by the daemon.  It never returns to main()
** in the daemon, but every time a connexion is received it forks a child,
** connects stdin, stdout, and stderr to the connexion, and returns to main()
** in the child.
*/
void get_connexion(int sockfd)
	{
	struct sigaction sig;
	sig.sa_handler = reapchild;			/* call reapchild on SIGCLD */
	sigemptyset(&sig.sa_mask);			/* block no additional sigs */
	#ifdef SA_RESTART
	sig.sa_flags = SA_RESTART;			/* restart interupted sys calls */
	#else
	sig.sa_flags = 0;
	#endif
	sigaction(SIGCHLD, &sig, NULL);

	for( ; ; )							/* loop until killed */
		{
		int pid;
		struct sockaddr_in cli_addr;
		unsigned int clilen;
		int newsockfd;

		DODEBUG_STANDALONE(("waiting for connexion"));

		clilen = sizeof(cli_addr);
		if( (newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen)) == -1 )
			{
			debug("be_server(): accept() failed, errno=%d (%s)", errno, gu_strerror(errno));
			continue;
			}

		DODEBUG_STANDALONE(("connection request from %s", inet_ntoa(cli_addr.sin_addr)));

		if((pid=fork()) == -1)	/* error, */
			{
			debug("be_server(): fork() failed, errno=%d (%s)", errno, gu_strerror(errno));
			}
		else if(pid == 0)		/* child */
			{
			am_child = TRUE;
			close(sockfd);
			if(newsockfd != 0) dup2(newsockfd, 0);
			if(newsockfd > 0) close(newsockfd);
			return;
			}

		/* Parent */
		DODEBUG_STANDALONE(("child %ld launched", (long)pid));
		close(newsockfd);
		}
	} /* end of be_server() */

/*
** This ties it all together.  It is called from main().
** Since the last thing it does is call get_connexion(),
** the parent never returns but the child does numberous times.
*/
void run_standalone(int server_port)
	{
	uid_t uid_ppr;
	gid_t gid_ppop;
	int fd;

	if(getuid() != 0)
		{
		fprintf(stderr, "Only root may start lprsrv in standalone mode.\n");
		exit(EXIT_DENIED);
		}

	gu_daemon(PPR_UMASK);
	chdir(HOMEDIR);

	half_demote_self(&uid_ppr, &gid_ppop);

	DODEBUG_MAIN(("standalone mode"));
	create_lock_file();

	DODEBUG_MAIN(("starting server on port %d", server_port));
	fd = bind_server(server_port, uid_ppr);

	fully_demote_self(uid_ppr, gid_ppop);

	get_connexion(fd);
	}

/*=============================================================================
** main() and its support routines:
=============================================================================*/

/*
** Command line options:
*/
static const char *option_chars = "pS:A:s:";
static const struct gu_getopt_opt option_words[] =
		{
		{"permissive", 'p', FALSE},
		{"arrest-interest-interval", 'A', TRUE},
		{"super-root-list", 'S', TRUE},
		{"standalone-port", 's', TRUE},
		{"help", 1000, FALSE},
		{"version", 1001, FALSE},
		{"hostname", 1002, FALSE},
		{"fully-qualify", 1003, TRUE},
		{(char*)NULL, 0, FALSE}
		} ;

/*
** Print how to use.  The argument will be either stdout or stderr.
*/
void help(FILE *out)
	{
	fputs("Valid switches:\n"
		"\t-p					  (permissive mode)\n"
		"\t-S <host1>[:<host2>]	  (hosts which may delete all jobs)\n"
		"\t-A <seconds>			  (arrest interest interval)\n"
		"\t-s <port>			  (run standalone)\n"
		"\t--version\n"
		"\t--help\n", out);
	}

/*
** main server loop,
** dispatch commands
*/
int main(int argc,char *argv[])
	{
	struct sockaddr_in client_address;
	unsigned int client_address_len;
	struct hostent *client_name;
	int permissive_mode = FALSE;
	int standalone_port = 0;
	int optchar;
	struct gu_getopt_state getopt_state;

	/*
	** Change to ppr's home directory.	That way we know
	** where our core dumps will go. :-)
	*/
	chdir(HOMEDIR);

	/*
	** Parse the command line options.	We use the parsing routine
	** in libppr.a.	 All of the parsing state is kept in the
	** structure getopt_state.
	*/
	gu_getopt_init(&getopt_state, argc, argv, option_chars, option_words);
	while( (optchar=ppr_getopt(&getopt_state)) != -1 )
		{
		switch(optchar)
			{
			case 'p':
				permissive_mode = TRUE;
				break;

			case 'S':
				{
				char *ptr = getopt_state.optarg;
				super_root_string = ptr;
				for( ; *ptr; ptr++) *ptr = tolower(*ptr);
				}
				break;

			case 'A':
				arrest_interesting_time = getopt_state.optarg;
				break;

			case 's':
				{
				if(strspn(getopt_state.optarg, "0123456789") == strlen(getopt_state.optarg))
					{
					standalone_port = atoi(getopt_state.optarg);
					}
				else
					{
					struct servent *service;
					if( (service=getservbyname(getopt_state.optarg, "tcp")) == (struct servent *)NULL )
						{
						fprintf(stderr, "Unknown port name: %s", getopt_state.optarg);
						exit(EXIT_SYNTAX);
						}
					standalone_port = ntohs(service->s_port);
					}
				}
				break;

			case 1000:					/* --help */
				help(stdout);
				exit(EXIT_OK);

			case 1001:					/* --version */
				puts(VERSION);
				puts(COPYRIGHT);
				puts(AUTHOR);
				exit(EXIT_OK);

			case 1002:					/* --hostname */
				{
				const char *retval = get_full_hostname();
				if(retval != (char*)NULL)
					{
					printf("%s\n", retval);
					exit(EXIT_OK);
					}
				exit(EXIT_NOTFOUND);
				}

			case 1003:					/* --fully-qualify */
				{
				const char *retval = fully_qualify_hostname(getopt_state.optarg);
				if(retval != (char*)NULL)
					{
					printf("%s\n", retval);
					exit(EXIT_OK);
					}
				exit(EXIT_NOTFOUND);
				}

			case '?':					/* help or unrecognized switch */
				fprintf(stderr, "Unrecognized switch: %s\n\n", getopt_state.name);
				help(stderr);
				exit(EXIT_SYNTAX);

			case ':':					/* argument required */
				fprintf(stderr, "The %s option requires an argument.\n", getopt_state.name);
				exit(EXIT_SYNTAX);

			case '!':					/* bad aggreation */
				fprintf(stderr, "Switches, such as %s, which take an argument must stand alone.\n", getopt_state.name);
				exit(EXIT_SYNTAX);

			case '-':					/* spurious argument */
				fprintf(stderr, "The %s switch does not take an argument.\n", getopt_state.name);
				exit(EXIT_SYNTAX);

			default:					/* missing case */
				fprintf(stderr, "Missing case %d in switch dispatch switch()\n", optchar);
				exit(EXIT_INTERNAL);
				break;
			}
		}

	/*
	** If we should run in standalone mode, do it now.
	** The parent will never return from this function call
	** but the children will.
	*/
	if(standalone_port)
		run_standalone(standalone_port);

	DODEBUG_MAIN(("connexion received"));

	/*
	** This must be done first thing!
	** INETD's only guarantee is that
	** stdin will be connected to the socket.
	*/
	dup2(0, 1);
	dup2(0, 2);

	/* Learn the address of the one we are talking to. */
	client_address_len = sizeof(client_address);
	if( getpeername(0, (struct sockaddr *)&client_address, &client_address_len) == -1 )
		fatal(1, "getpeername() failed, errno=%d (%s)", errno, gu_strerror(errno) );

	/* Get the name of the one we are talking to. */
	if( (client_name=gethostbyaddr((char*)&client_address.sin_addr,sizeof(client_address.sin_addr),AF_INET)) != (struct hostent*)NULL)
		{
		clipcopy(client_dns_name, client_name->h_name, LPR_MAX_H);
		}
	else
		{
		char *ptr;
		debug("gethostbyaddr() failed, errno=%d (%s)", errno, gu_strerror(errno));
		strcpy(client_dns_name, inet_ntoa(client_address.sin_addr));
		for(ptr=client_dns_name; *ptr; ptr++) *ptr = tolower(*ptr);
		}

	/* Make a note of the port the request is coming from. */
	client_port = ntohs(client_address.sin_port);

	DODEBUG_MAIN(("connexion is from %s (%s), port %d", client_dns_name, inet_ntoa(client_address.sin_addr), client_port));

	/*
	** Known clients must send from reserved ports.
	** Unknown clients are only allowed if we are
	** running in permissive mode.
	*/
	if( authorized(client_dns_name) )
		{
		if( client_port > 1024 )
			fatal(1, "Connexion not from a reserved port");
		}
	else if(! permissive_mode)
		{
		fatal(1, "not authorized: %s", client_dns_name);
		}

	/* Do zero or one commands and exit. */
	if(lprsrv_getline())
		{
		switch(line[0])
			{
			case 1:									/* ^A */
				DODEBUG_MAIN(("start printer: ^A%s", line+1));
				break;
			case 2:									/* ^B */
				DODEBUG_MAIN(("receive job: ^B%s", line+1));
				take_job(line);
				break;
			case 3:									/* ^C */
				DODEBUG_MAIN(("short queue: ^C%s", line+1));
				show_jobs(line);					/* This never returns */
				break;
			case 4:									/* ^D */
				DODEBUG_MAIN(("long queue: ^D%s", line+1));
				show_jobs(line);					/* This never returns */
				break;
			case 5:									/* ^E */
				DODEBUG_MAIN(("remove: ^E%s", line+1));
				remove_jobs(line);					/* Remove jobs */
				break;
			default:								/* what can we do? */
				fatal(1, "unrecognized command: ^%c%s", line[0]+'@', line+1);
			}
		} /* end of main if */

	return 0;
	} /* end of main() */

/* end of file */

