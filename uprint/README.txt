uprint/README.txt
22 September 2004

This directory contains programs which emulate programs commonly used to
submit jobs on Unix systems.  These program do most of their work by calling
the Uprint library which may be found in ../libuprint.

If you use these programs, you might want to rename lpr, lpq, lprm, lpc, lp,
cancel, and lpstat.  You can then make links in /usr/bin and /usr/sbin to the 
uprint versions in ~ppr/bin.  If you edit /etc/ppr/uprint.conf and run
~ppr/bin/uprint-newconf it will do all this for you.

