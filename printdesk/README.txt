mouse:~ppr/src/printdesk/README.txt
7 February 2000.

This directory contains the PPR's PrintDesk library.  This library defines
Perl 5 GUI objects for for monitoring PPR queues and printers and controling
them.  

These GUI objects are used in a little application called ppr-panel.  It
still some bugs but it is still interesting.  The ppr-panel program has been
tested with Tk400.202 and Tk800.012.  It doesn't quite work with Tk800.000
because of a bug in Tk800.000.

In this directory you will also find a little program called "ppr-chooser".
You can use this to select an AppleTalk printer.  If will write the address
of the chosen printer to stdout.  The address will be expressed in the format
required by PPR's "atalk" interface program.
