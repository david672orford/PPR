mouse:~ppr/src/ppad/README.txt
7 February 2006

This directory contains the source code for ppad.  Ppad is PPR's printer
administrator's utility.

ppad.h
	Included by all of those below
ppad.c
	Command dispatcher and a few utility routines
dispatch_table.xsl
	XSLT style sheet for generating dispatch_table.c from XML
	code embedded in the C source files
ppad_conf.c
	Functions for manipulating printer, group, and alias 
	configuration files
ppad_util.c
	Additional utility routines
ppad_printer.c
	Implementation of subcommands related to printers
ppad_group.c
	Implementation of subcommands related to groups
ppad_alias.c
	Implementation of subcommands related to aliases
ppad_media.c
	Implementation of subcommands related to media
ppad_ppd.c
	Implementation of subcommands related to PPD files


