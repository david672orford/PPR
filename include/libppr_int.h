/*
** mouse:~ppr/src/include/libppr_int.h
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
** Last modified 16 October 2003.
*/

#ifndef LIBPPR_INT_H
#define LIBPPR_INT_H 1

#include "gu.h"

/*
** This is a structure into which interface arguments are stored
** for handy reference later.  The storing is done by
** interface_parse_argv().	To the right of each member is a sample
** of what it might contain.  If you want a real explaination of
** the significance of the interface options, looks at the
** appendix "Requirements for an Interface" in "PPR, a Print Spooler for
** PostScript".
*/
struct INT_CMDLINE
	{
	gu_boolean probe;					/* TRUE if --probe used */
	const char *int_name;				/* example: "interfaces/atalk" */
	const char *int_basename;			/* example: "atalk" */
	const char *printer;				/* example: "myprn" */
	const char *address;				/* example: "My Laser Printer:LaserWriter@Computing Center" */
	const char *options;				/* example: "idle_status_interval=60 open_retries=5" */
	int jobbreak;						/* example: 1 (signal) */
	gu_boolean feedback;				/* example: 1 (True) */
	enum CODES codes;					/* example: 3 (Binary) */
	const char *jobname;				/* example: "mouse:myprn-1001.0(mouse)" */
	const char *routing;				/* example: "Call David Chappell at 2114 when ready" */
	const char *forline;				/* example: "David Chappell" */
	const char *barbarlang;				/* example: "" (PostScript) */
	const char *title;					/* example: "My Print Job" */
	} ;

extern struct INT_CMDLINE int_cmdline;

/*
** This stuff is for interfaces that use TCP to connect to the printer.
** It is excluded if the necessary IP include files haven't been included.
*/
#ifdef INADDR_NONE
struct TCP_CONNECT_OPTIONS
	{
	int timeout;
	int sndbuf_size;
	int refused_retries;
	gu_boolean refused_engaged;
	};

int int_tcp_connect_option(const char name[], const char value[], struct OPTIONS_STATE *o, struct TCP_CONNECT_OPTIONS *options);
void int_tcp_parse_address(const char address[], int default_port, struct sockaddr_in *printer_addr);
int int_tcp_open_connexion(const char address[], struct sockaddr_in *printer_addr, struct TCP_CONNECT_OPTIONS *options, void (*status_function)(void *), void *status_obj);
#endif

/*
** Prototypes for other functions in libppr which are used
** only by interfaces.
*/
void int_cmdline_set(int argc, char *argv[]);
void int_addrcache_save(const char printer[], const char interface[], const char address[], const char resolution[]);
char *int_addrcache_load(const char printer[], const char interface[], const char address[], int *age);
void int_copy_job(int portfd, int idle_status_interval, void (*fatal_prn_err)(int err), void (*send_eoj_funct)(int fd), void (*snmp_function)(void *address), void *address, int snmp_status_interval);
void print_pap_status(const unsigned char *status);
void int_exit(int exitvalue)
	#ifdef __GNUC__
	__attribute__ (( noreturn ))
	#endif
	;
void int_debug(const char format[], ...)
	#ifdef __GNUC__
	__attribute__ ((format (printf, 1, 2)))
	#endif
	;

#endif

/* end of file */

