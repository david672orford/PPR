mouse:~ppr/src/libscript/README.txt
30 June 2000

This directory contains library functions for shell scripts and Perl scripts.
It also contains some programs which allow shell scripts to call functions
in libppr.

====================
 ppr_conf_query
====================

This is used the query the ppr.conf file.

====================
 alert
====================

This program is a wrapper for alert().  It is used to post alerts to a
printer's alerts log.

====================
 time_elapsed
====================

This program prints an English message telling how long ago a Unix format time
was.  The time (in seconds since the Epoc) is the first argument.  If their is
a second argument than it must be a number of seconds.  If the second argument
is present, than nothing will be printed for times no longer in the past than
the second argument indicates.

====================
 getpwnam
 getgrnam
 getservbyname
====================

These programs are very simple wrappers around the C library routines of the
same names.

