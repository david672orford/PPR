mouse:~ppr/src/browers/README.txt
6 December 2002

In this directory we are creating little programs which can list available
printers in certain realms.  If one of these programs is run without 
arguments, it will display a list of the realms which it can search.  It
can then be rerun with name name of one of the realms as and argument
and it will return a list of the printers in that realm (or ports on
which it thinks printers might exist).

Here is an example:

$ ./parallel.tcl
Parallel Ports
$ ./parallel.tcl "Parallel Ports"
[0]
comment=Parallel Port 0
manufacturer=EPSON
model=Stylus C62
interface=simple,/dev/lp0
interface=parallel,/dev/lp0

[1]
comment=Parallel Port 1
manufacturer=EPSON
model=Stylus C62
interface=simple,/dev/lp1
interface=parallel,/dev/lp1

These programs will be called from an as yet unwritten program which will
allow the user to choose a printer for which to create a queue.  Currently 
this kind of browsing is supported only in the WWW interface and there
only for AppleTalk.

