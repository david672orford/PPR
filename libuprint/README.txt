mouse:~ppr/src/libuprint/README.txt
18 February 2003

This directory contains a library of functions for submitting jobs to various
print spoolers and for sending to remote spoolers using the LPR/LPD protocol.

The PPR lpr/lpd compatible server (lprsrv) uses this library.  The old lprsrv
(olprsrv) makes only limited use of this library.

In the directory ../uprint/ you will find the programs uprint-lp, uprint-lpr,
uprint-lpq, and uprint-lprm all use this library to get their work done.

An attempt has been made to keep the reliance of this code on PPR to a 
minimum.  For example, it calls only a few functions in libppr.a.  That way 
it could be separated if someone wants to.



* uprint_print.c
    This module contains model code for dispatching a job.  It calls code in 
    the uprint_print_*.c module to build print commadn argument lists which 
    it later execute (except for RFC 1179 jobs which are sent by their
    module).

    The uprint-lp and uprint-lpr commands use this module.  However lprsrv
    does not.  Instead it calls the sysv, bsd, and ppr module directly to
    build command lines which it then executes itself.


* uprint_print_sysv.c
    Contains code to build a command line for System V's lp.

* uprint_print_bsd.c
    Contains code to build a command line for BSD's lpr.

* uprint_print_ppr.c
    Contains code to build a command line for PPR's ppr.

* uprint_print_rfc1179.c
    Contains code to send the file using the lpr protocol
    described in RFC 1179.

