/*
** mouse:~ppr/src/interfaces/parallel.h
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
** Last modified 22 November 1999.
*/

/* Those interface options which are not parallel port settings: */
struct OPTIONS {
	int idle_status_interval;
	gu_boolean reset_before;
	gu_boolean reset_on_cancel;
	} ;

/* Define OS independent names for the parallel port
   status lines. */
#define PARALLEL_PORT_OFFLINE 1
#define PARALLEL_PORT_PAPEROUT 2
#define PARALLEL_PORT_FAULT 4

void parallel_port_setup(int fd, const struct OPTIONS *options);
void parallel_port_reset(int fd);
int parallel_port_status(int fd);
void parallel_port_cleanup(int fd);

/* end of file */

