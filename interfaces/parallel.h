/*
** mouse:~ppr/src/interfaces/parallel.h
** Copyright 1995--2006, Trinity College Computing Center.
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
** Last modified 15 February 2006.
*/

/* Those interface options which are not parallel port settings: */
struct OPTIONS {
	int idle_status_interval;
	int status_interval;
	gu_boolean reset_before;
	gu_boolean reset_on_cancel;
	} ;

/* Define OS independent names for the parallel port
   status lines. */
#define PARALLEL_PORT_OFFLINE 1
#define PARALLEL_PORT_PAPEROUT 2
#define PARALLEL_PORT_FAULT 4
#define PARALLEL_PORT_BUSY 8

void parallel_port_setup(int fd, const struct OPTIONS *options);
void parallel_port_reset(int fd);
int parallel_port_status(int fd);
void parallel_port_error(const char syscall[], int fd, int error_number);
void parallel_port_cleanup(int fd);
int parallel_port_probe(const char address[]);

/* end of file */

