mouse:~ppr/src/printdesk/README.txt
18 March 2003

This directory contains a set of GUI widgets for user (non-administrator)
operations involving PPR.  The program ppr-panel lauches a small program
which uses these widgets to print files, show queues, monitor printers, and
start and stop printers.  These widgets predate the web interface, in fact
parts of the web interface are modeled on them.  In some areas, such as
printer icons, the web interface has exceeded these widgets, in others, such
as real-time status, these widgets still have the lead.

In this directory you will also find a little program called "ppr-chooser".
You can use this to select an AppleTalk printer.  If will write the address
of the chosen printer to stdout.  The address will be expressed in the format
required by PPR's "atalk" interface program.
