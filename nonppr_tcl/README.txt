mouse:~ppr/src/tcl/README.txt
Last modified 19 January 2005.

This directory contains a modified version of Tcl 7.4p2.  The modifications
are as follows:

* Removed documentation.

* Removed use of sprintf()

* Removed use of mktemp().

* Removed history command.

* Removed lots of messy code amd #ifdefs for compatibility with non-POSIX 
  systems (not finished yet).

* Removed sccs line at the top of each file.

* Converted prototypes to ANSI C (not finished yet).

* Fixed tests/lsort.test for strtod() which can parse 0x40.

* tclAppInit.c: replaced with version which includes PPR Tcl commands
  (ppr-tclsh.c)

