7 February 2000.

This directory contains the PPR's Printman library.  This library defines
Perl 5 objects for controlling PPR print queues and creating X-Windows GUI
elements for monitoring PPR queues and printers and controling them.  Some of
these library functions are used by the CGI scripts in the directory "../www/".
The X-Windows GUI elements require Perl/Tk.

In this directory there is also a pretty little GUI for watching PPR queues
and printers.  It is called "ppr-panel".  It still some bugs but it is still
interesting.  The ppr-panel program has been tested with Tk400.202 and
Tk800.012.  It doesn't quite work with Tk800.000 because of a bug in Tk800.000.

In this directory you will also find a little program called "ppr-chooser".
You can use this to select an AppleTalk printer.  If will write the address
of the chosen printer to stdout.  The address will be expressed in the format
required by PPR's "atalk" interface program.

