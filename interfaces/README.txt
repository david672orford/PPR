20 June 2001.

This directory contains printer interface programs.  A printer interface
program is launched by pprdrv.  Its job is to communicate with the printers.
The interface program will read data from stdin and send it to the printer.
Any data it receives from the printer it should write to stdout.  If it
encounters a problem, it should use alert(), valert() or the program lib/alert
to write an error message to the printer's alert log.  It should then exit with
an appropriate error code from interface.h or interface.sh.

How the interface communicates with the printer is entirely up to it.  You will
notice that a different interface program is provided for each communication
method.

In the directory ../libppr/int_*.c you will find functions useful to interfaces.

The interface program receives a number of command line options.  These include
a printer address and an options list.  The syntax of these two parameters
varies from interface to interface.  A more complete description of the
command line and how an interface ought to work can be found in "PPR, a
Print Spooler for PostScript" which may be found in the PPR documentation
distribution.  Many of the interface programs use the function
int_cmdline_set() to read the command line options into the global structure
int_cmdline.

The files feedback_test[1-3].ps are for use when testing interfaces.  An
interface which supports feedback should be capable of capturing all of the
output from each one of them.

The main part of the interface program for parallel ports is in the file
parallel.c.  Operating system specific routines can be found in the
parallel_*.c files.  The correct one is selected by the PARALLEL variable
defined in ../include/global.mk.  Operating systems for which a module has not
yet been written use parallel_generic.c which contains empty functions.  If
you want to make a module for the operating system you use, look in
parallel_generic.c for descriptions of the functions and at the modules for
other operating systems for practical examples.

