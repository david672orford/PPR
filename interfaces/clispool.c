/*
** mouse:~ppr/src/interfaces/clispool.c
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
** Last modified 23 January 2004.
*/

/*
** PPR's interface to AT&T LANMAN style client spooled printers.
*/

#include "before_system.h"
#include <tiuser.h>
#include <fcntl.h>
#include <sys/utsname.h>		/* for uname */
#include <ctype.h>				/* for toupper() */
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/wait.h>
#include "gu.h"
#include "global_defines.h"
#include "interface.h"

/* Optional debugging */
#if 0
#define DEBUG(a) debug a
#else
#define DEBUG(a)
#endif

/* file for debug log */
#define CLISPOOL_LOGFILE LOGDIR"/interface_clispool"

/* device to open for NetBIOS */
#define CIRCUIT "/dev/starlan"

/* program to run to get remote status */
#define REMSTAT "/var/opt/slan/bin/remstat"

#define SPOOL_AREA_TEMPLATE "\\\\%s.serve\\pprclipr"
#define FILE_NAME_TEMPLATE "JOB%ld"				/* notice upper case! */
#define FULL_FILE_NAME_TEMPLATE VAR_SPOOL_PPR"/pprclipr/job%ld"

/* AT&T System V Release 4.0 WGS doesn't have these. */
#define NEEDS_TLI_PROTOS 1

#define WORD short int
#define BYTE char

struct clipr_request
		{
		WORD signiture;					/* 0x1988 */
		BYTE clispool_errno;			/* clispool error code */
		BYTE print_errno;				/* spooler return code */
		WORD extra_code;				/* set to -1 if submitted to print */
		BYTE spool_area[40];			/* server name and spool area */
		BYTE drive[2];					/* "x:" */
		BYTE file_name[16];				/* name of file to print */
		} ;

#define CLISPOOL_OK 0			/* job printed ok */
#define CLISPOOL_STOPPED 1		/* CLISPOOL is disabled */
#define CLISPOOL_CANTLINK 2		/* couldn't link to transfer area */
#define CLISPOOL_DENIED 3		/* this spooler not allowed to connect */
#define CLISPOOL_PRINT_ERROR 4	/* print wouldn't accept the job */

/* If the TLI prototypes are missing, supply them. */
#ifdef NEEDS_TLI_PROTOS
extern int t_errno;
int t_open(char *path, int oflag, struct t_info *info);
int t_rcv(int fildes, char *buf, unsigned nbytes, int *flags);
int t_snd(int fildes, char *buf, unsigned nbytes, int flags);
int t_free(char *ptr, int struct_type);
char *t_alloc(int fildes, int struct_type, int fields);
int t_bind(int fildes, struct t_bind *req, struct t_bind *ret);
int t_close(int fildes);
int t_connect(int fildes, struct t_call *sndcall, struct t_call *rcvcall);
#endif

/* global variables */
char *printer_name;				/* the name of the printer */
char *printer_address;			/* the name of the client to call */
char full_file_name[MAX_PPR_PATH];		/* name of temporary file */

/*
** Write lines to the debug file.
*/
void debug(char *string, ... )
	{
	va_list va;
	FILE *file;

	va_start(va,string);
	if((file = fopen(CLISPOOL_LOGFILE, "a")))
		{
		fprintf(file,"DEBUG: (%ld) ",(long)getpid());
		vfprintf(file,string,va);
		fprintf(file,"\n");
		fclose(file);
		}
	va_end(va);
	} /* end of debug() */

/*
** Fatal error handler.
*/
void fatal(int rval, const char *string, ... )
	{
	va_list va;
	FILE *file;

	va_start(va,string);

	if((file = fopen(CLISPOOL_LOGFILE,"a")))  /* log the error */
		{
		fprintf(file,"FATAL: (%ld) ", (long)getpid());
		vfprintf(file,string,va);
		fprintf(file,"\n");
		fclose(file);
		}

	valert(printer_name,TRUE,string,va);		   /* inform the operator */

	va_end(va);

	if(full_file_name[0])				/* if xfer file created, */
		unlink(full_file_name);			/* remove it now */

	exit(rval);			/* fatal means die */
	}

/*
** Entry point.
*/
int int_main(int argc, char *argv[])
	{
	int endpoint;				/* network file handles */
	char local_name[17];		/* our NetBIOS name */
	char remote_name[17];		/* client's NetBIOS name */
	struct t_bind *req;
	struct t_call *sndcall;
	int x;
	int nbytes;					/* for t_snd() and t_rcv() */
	int flags;					/* for t_rcv() */
	struct utsname uname_data;	/* structure containing system name and other things */
	pid_t pid;					/* our process id */
	struct clipr_request request;/* the request packet we send to client */
	void *copybuf;
	int copyfd;

	full_file_name[0] = '\0';	/* for fatal() */

	if(argc >= 2 && strcmp(argv[1], "--probe") == 0)
		{
		fatal(EXIT_PRNERR_NORETRY_BAD_SETTINGS, "The interface program \"%s\" does not support probing.\n", argv[0]);
		}

	if(argc < 3)
		fatal(EXIT_PRNERR_NORETRY_BAD_SETTINGS, "Interface invokation error: insufficient parameters.");

	printer_name = argv[1];				/* assign the arguments */
	printer_address = argv[2];			/* to global variables */

	chdir(HOMEDIR);						/* change to PPR's home directory */

	/* get the system name which we will use in building the */
	/* local network name and in building the share area name */
	if(uname(&uname_data) == -1)
		fatal(EXIT_PRNERR_NORETRY, "Internal interface error: uname() failed, errno=%d (%s).", errno, strerror(errno));
	#ifdef DEBUG
	debug("uname is \"%s\"",uname_data.nodename);
	#endif

	/* get the process id for file names */
	pid = getpid();
	#ifdef DEBUG
	debug("pid=%d",pid);
	#endif

	/* construct the local network name */
	snprintf(local_name, sizeof(local_name), "%s%ld", uname_data.nodename, pid);
	while( (int)strlen(local_name) < (int)16 )
		strcat(local_name," ");

	/* construct the remote network name */
	for(x=0; x<14 && printer_address[x]; x++)
		remote_name[x]=toupper(printer_address[x]);
	remote_name[x++]='.';
	remote_name[x++]='P';
	while(x<16)
		remote_name[x++]=' ';
	remote_name[x]=(char)NULL;

	#ifdef DEBUG
	debug("local name=\"%s\", remote name=\"%s\"",local_name,remote_name);
	#endif

	/* fill in the request packet */
	request.signiture=0x1988;
	request.clispool_errno=0;
	request.print_errno=0;
	request.extra_code=0;
	snprintf(request.spool_area, sizeof(request.spool_area), SPOOL_AREA_TEMPLATE, uname_data.nodename);
	for(x=0;request.spool_area[x];x++)							/* Convert path name */
		request.spool_area[x]=toupper(request.spool_area[x]);	/* to upper case and */
	for( ;x<sizeof(request.spool_area); x++)					/* fill extra space with nulls */
		request.spool_area[x]=(char)NULL;						/* because CLISPOOL is fussy. */
	request.drive[0]='J';
	request.drive[1]=':';
	snprintf(request.file_name, sizeof(request.file_name), FILE_NAME_TEMPLATE, (long)pid);
	#ifdef DEBUG
	debug("spool area is \"%s\", file is \"%s\"",request.spool_area,request.file_name);
	#endif

	/* build the name of the temporary file */
	ppr_fnamef(full_file_name, FULL_FILE_NAME_TEMPLATE, (long)pid);
	#ifdef DEBUG
	debug("temporary file is \"%s\"", full_file_name);
	#endif

	/* Copy stdin to a temporary file */
	if( (copybuf=malloc((size_t)4096)) == (void*)NULL)
		fatal(EXIT_PRNERR,"Internal interface error: malloc() failed, errno=%d (%s).", errno, strerror(errno));

	if((copyfd = open(full_file_name, O_WRONLY | O_CREAT, UNIX_755)) == -1)
		fatal(EXIT_PRNERR, "Internal interface error: open() failed for transfer file, errno=%d (%s).", errno, strerror(errno));

	while( (nbytes=read(0,copybuf,4096)) )
		{
		if(nbytes==-1)
			fatal(EXIT_PRNERR, "Internal interface error: read() failed, errno=%d (%s).",errno, strerror(errno));
		if(write(copyfd,copybuf,nbytes) != nbytes)
			{
			if(nbytes==-1)
				fatal(EXIT_PRNERR, "Error writing transfer file: write() failed, errno=%d (%s)", errno, strerror(errno));
			else
				fatal(EXIT_PRNERR, "Disk full error writing transfer file.");
			}
		}

	if( close(copyfd) == -1 )
		fatal(EXIT_PRNERR_NORETRY, "close() failed, errno=%d",errno);

	free(copybuf);

	/* open an endpoint */
	#ifdef DEBUG
	debug("opening endpoint");
	#endif
	if( (endpoint=t_open(CIRCUIT,O_RDWR,(struct t_info*)NULL)) == -1 )
		fatal(EXIT_PRNERR, "Interface internal error: t_open() failed, t_errno=%d.", t_errno);

	/* bind the local name to the endpoint */
	#ifdef DEBUG
	debug("binding local name to endpoint");
	#endif
	if( (req=(struct t_bind*)t_alloc(endpoint,T_BIND,T_ALL)) == (struct t_bind*)NULL )
		fatal(EXIT_PRNERR, "Interface internal error: t_alloc() failed, t_errno=%d.", t_errno);
	strcpy(req->addr.buf,local_name);	/* copy the local */
	req->addr.len=strlen(local_name);	/* name into a structure */
	req->qlen=0;
	if( t_bind(endpoint,req,(struct t_bind*)NULL) == -1 )
		fatal(EXIT_PRNERR, "Interface internal error: t_bind() failed, t_errno=%d.", t_errno);
	if(t_free((char *)req,T_BIND) == -1)
		fatal(EXIT_PRNERR, "Interface internal error: t_free() failed, t_errno=%d.", t_errno);

	/* connect to remote name */
	#ifdef DEBUG
	debug("connecting to remote computer");
	#endif
	if( (sndcall=(struct t_call*)t_alloc(endpoint,T_CALL,T_ADDR)) == (struct t_call*)NULL )
		fatal(EXIT_PRNERR, "t_alloc() failed, t_errno=%d", t_errno);
	strcpy(sndcall->addr.buf,remote_name);
	sndcall->addr.len=strlen(remote_name);
	sndcall->opt.len=0;
	sndcall->udata.len=0;
	if( t_connect(endpoint,sndcall,(struct t_call*)NULL) == -1 )
		{
		if(t_errno==TLOOK)				/* if not there or not listening */
			{
			int waitstat;
			int fd;

			unlink(full_file_name);		/* remove the transfer file */
			full_file_name[0] = '\0';

			if( (fd=open("/dev/null",O_RDWR)) != -1 )	/* open /dev/null */
				{										/* for stdi/o for remstat */
				switch(fork())
					{
					case -1:					/* error */
						close(fd);
						/* fatal(EXIT_PRNERR,"Internal error: fork() failed attempting to execute remstat."); */
						exit(EXIT_STARVED);

					case 0:						/* child */
						dup2(fd,0);
						dup2(fd,1);
						dup2(fd,2);
						if(fd>2)
							close(fd);
						execl(REMSTAT,REMSTAT,"-n",remote_name,(char*)NULL);
						_exit(255);

					default:			/* parent */
						close(fd);
						wait(&waitstat);
						if(!WIFEXITED(waitstat))
							fatal(EXIT_PRNERR, "Internal error: remstat did not exit normally.");
						if(WEXITSTATUS(waitstat)==255)
							fatal(EXIT_PRNERR, "Internal error: remstat not found.");
						if(WEXITSTATUS(waitstat)!=0)	/* if machine not found */
							fatal(EXIT_PRNERR, "Client spooler is turned off or unreachable.");
					}
				} /* end of if /dev/null opened ok */
			return EXIT_ENGAGED;
			}
		else
			{
			fatal(EXIT_PRNERR, "t_connect() failed, t_errno=%d", t_errno);
			}
		}
	if(t_free((char*)sndcall,T_CALL) == -1)
		fatal(EXIT_PRNERR, "t_free() failed, t_errno=%d", t_errno);

	/* send the request packet to the client */
	#ifdef DEBUG
	debug("sending request packet");
	#endif
	if( (nbytes=t_snd(endpoint,(char*)&request,sizeof(request),0)) != sizeof(request) )
		{
		if(nbytes==-1)
			fatal(EXIT_PRNERR, "t_snd() failed, errno=%d (%s).", errno, strerror(errno));
		else
			fatal(EXIT_PRNERR, "t_snd() failed, return value=%d", nbytes);
		}

	/* receive the response */
	#ifdef DEBUG
	debug("waiting for response");
	#endif
	if( (nbytes=t_rcv(endpoint,(char*)&request,sizeof(request),&flags)) != sizeof(request) )
		{
		if(nbytes==-1)
			fatal(EXIT_PRNERR, "t_rcv() failed, errno=%d (%s)", errno, strerror(errno));
		else
			fatal(EXIT_PRNERR, "t_rcv() failed, return value=%d", nbytes);
		}

	/* undo network stuff */
	#ifdef DEBUG
	debug("disconnecting");
	#endif
	if( t_close(endpoint) == -1 )
		fatal(EXIT_PRNERR, "t_close() failed, t_errno=%d", t_errno);

	/* remove the temporary file */
	#ifdef DEBUG
	debug("removing temporary file");
	#endif
	unlink(full_file_name);
	full_file_name[0] = '\0';

	/* evaluate the response */
	#ifdef DEBUG
	debug("clispool_errno=%d, print_errno=%d, extra_code=%d",request.clispool_errno,request.print_errno,request.extra_code);
	#endif

	switch(request.clispool_errno)
		{
		case CLISPOOL_OK:
			#ifdef DEBUG
			debug("printing done");
			#endif
			return EXIT_PRINTED;
		case CLISPOOL_STOPPED:
			alert(printer_name, TRUE, "CLISPOOL is temporarily disabled.");
			return EXIT_ENGAGED;
		case CLISPOOL_CANTLINK:
			alert(printer_name, TRUE, "The client can't link to \"%s\", DOS error %d.",request.spool_area,request.clispool_errno);
			return EXIT_PRNERR;
		case CLISPOOL_DENIED:
			alert(printer_name, TRUE, "Client has denied its services to this spooler.");
			return EXIT_PRNERR_NORETRY_ACCESS_DENIED;
		case CLISPOOL_PRINT_ERROR:
			alert(printer_name, TRUE, "Spooler process on client refuses to accept file,");
			switch(request.print_errno)
				{
				case 0:
					alert(printer_name, FALSE, "no reason given.");
					return EXIT_PRNERR;			/* this seems best */
				case 1:
					alert(printer_name, FALSE, "claiming it was asked to perform an invalid function.");
					return EXIT_PRNERR_NORETRY;
				case 2:
					alert(printer_name, FALSE, "because the file was not found.");
					return EXIT_PRNERR_NORETRY;
				case 3:
					alert(printer_name, FALSE, "because the path was not found.");
					return EXIT_PRNERR_NORETRY;
				case 4:
					alert(printer_name, FALSE, "because the client computer has too many files open.");
					return EXIT_PRNERR;
				case 5:
					alert(printer_name, FALSE, "because it was denied access to \"%s\".",request.spool_area);
					return EXIT_PRNERR;			/* this seems best */
				case 8:
					alert(printer_name, FALSE, "because its queue is full.");
					return EXIT_PRNERR;
				case 9:
					alert(printer_name, FALSE, "claiming that its API is busy.	(This shouldn't happen.)");
					return EXIT_PRNERR;			/* hope it will go away */
				case 0xC:
					alert(printer_name, FALSE, "claiming that the file name is too long.");
					return EXIT_PRNERR_NORETRY;
				case 0xF:
					alert(printer_name, FALSE, "claiming that it was asked to print a file on an invalid drive.");
					return EXIT_PRNERR;			/* CLISPOOL might say this, not CLIPRINT */
				default:
					alert(printer_name, FALSE, "because DOS error number %d has occured.",request.print_errno);
					return EXIT_PRNERR_NORETRY;
				}
		default:
			alert(printer_name, TRUE, "Client has returned an undefined error code: %d.",request.clispool_errno);
			return EXIT_PRNERR_NORETRY;
		}
	} /* end of main() */

/* end of file */
