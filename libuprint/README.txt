mouse:~ppr/src/libuprint/README.txt
29 June 2000

This directory contains a library of functions for submitting jobs to various
print spoolers and for sending to remote spoolers using the LPR/LPD protocol.

The PPR lpr/lpd compatible server (lprsrv) uses this library.  The old lprsrv
(olprsrv) makes only limited use of this library.

In the directory ../uprint you will find the programs uprint-lp, uprint-lpr,
uprint-lpq, and uprint-lprm all use this library to get their work done.

An attempt has been made to keep the reliance of this code on PPR to a minium.
For example, it does calls few functions in libppr.a.  That way it could be
separated if someone wants to.

