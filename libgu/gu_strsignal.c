/*
** mouse:~ppr/src/libgu/gu_strsignal.c
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
** Last modified 13 December 2004.
*/

/*
** This function will return a signal name if it is passed a
** signal number.  This is a commonly used function that happens
** to be missing from some C libraries.
*/

#include "config.h"
#include <string.h>
#include <signal.h>
#include "gu.h"
#undef strsignal

char *gu_strsignal(int signum)
	{
	#ifdef HAVE_STRSIGNAL
	return strsignal(signum);
	#else
	switch(signum)
		{
		#ifdef SIGHUP
		case SIGHUP:
			return "SIGHUP";
		#endif
		#ifdef SIGINT
		case SIGINT:
			return "SIGINT";
		#endif
		#ifdef SIGQUIT
		case SIGQUIT:
			return "SIGQUIT";
		#endif
		#ifdef SIGILL
		case SIGILL:
			return "SIGILL";
		#endif
		#ifdef SIGTRAP
		case SIGTRAP:
			return "SIGTRAP";
		#endif
		#ifdef SIGABRT
		case SIGABRT:
			return "SIGABRT";
		#endif
		#ifdef SIGEMT
		case SIGEMT:
			return "SIGEMT";
		#endif
		#ifdef SIGFPE
		case SIGFPE:
			return "SIGFPE";
		#endif
		#ifdef SIGKILL
		case SIGKILL:
			return "SIGKILL";
		#endif
		#ifdef SIGBUS
		case SIGBUS:
			return "SIGBUS";
		#endif
		#ifdef SIGSEGV
		case SIGSEGV:
			return "SIGSEGV";
		#endif
		#ifdef SIGSYS
		case SIGSYS:
			return "SIGSYS";
		#endif
		#ifdef SIGPIPE
		case SIGPIPE:
			return "SIGPIPE";
		#endif
		#ifdef SIGALRM
		case SIGALRM:
			return "SIGALRM";
		#endif
		#ifdef SIGTERM
		case SIGTERM:
			return "SIGTERM";
		#endif
		#ifdef SIGUSR1
		case SIGUSR1:
			return "SIGUSR1";
		#endif
		#ifdef SIGUSR2
		case SIGUSR2:
			return "SIGUSR2";
		#endif
		#ifdef SIGCHLD			/* POSIX version */
		case SIGCHLD:
			return "SIGCHLD";
		#else
		#ifdef SIGCLD			/* old version */
		case SIGCLD:
			return "SIGCLD";
		#endif
		#endif
		#ifdef SIGPWR
		case SIGPWR:
			return "SIGPWR";
		#endif
		#ifdef SIGWINCH
		case SIGWINCH:
			return "SIGWINCH";
		#endif
		#ifdef SIGURG
		case SIGURG:
			return "SIGURG";
		#endif
		#ifdef SIGPOLL			/* System V version */
		case SIGPOLL:
			return "SIGPOLL";
		#else
		#ifdef SIGIO			/* Berkeley version */
		case SIGIO:
			return "SIGIO";
		#endif
		#endif
		#ifdef SIGSTOP
		case SIGSTOP:
			return "SIGSTOP";
		#endif
		#ifdef SIGTSTP
		case SIGTSTP:
			return "SIGTSTP";
		#endif
		#ifdef SIGCONT
		case SIGCONT:
			return "SIGCONT";
		#endif
		#ifdef SIGTTIN
		case SIGTTIN:
			return "SIGTTIN";
		#endif
		#ifdef SIGTTOU
		case SIGTTOU:
			return "SIGTTOU";
		#endif
		#ifdef SIGVTALRM
		case SIGVTALRM:
			return "SIGVTALRM";
		#endif
		#ifdef SIGPROF
		case SIGPROF:
			return "SIGPROF";
		#endif
		#ifdef SIGXCPU
		case SIGXCPU:
			return "SIGXCPU";
		#endif
		#ifdef SIGXFSZ
		case SIGXFSZ:
			return "SIGXFSZ";
		#endif
		default:
			return "<OTHER SIGNAL>";
		}
	#endif
	} /* end of gu_strsignal() */

/* end of file */
