mouse:~ppr/src/ppop/README.txt
13 February 2003

This directory contains the source code for ppop.  ppop is PPR's printer
operator utility.  The code is devided into source files as follows:

ppop.h
	Included by all of those below
ppop.c
	The command dispatcher and some utility routines
ppop_cmds_listq.c
	Implementation of "ppop list" and similar commands for listing jobs
	in print queues
ppop_cmds_other.c
	Most other commands
ppop_modify.c
	Implementation of "ppop modify"

