mouse:~ppr/src/ipp/README.txt
7 April 2005

This directory contains PPR's experimental IPP server.

PPR will soon implement IPP 1.0 and 1.1 as well as the CUPS extensions.  It can
already work to some extent with many IPP clients intended for use with CUPS.
These include the CUPS command-line programs lp, lpr, lpq, lpstat, etc., as
well as KDE's printing subsystem, Openoffice.org, and Mozilla.

=== The Code ===

This program is a CGI 'script'.  It decodes the IPP request and either handles
the request itself or passes it on to pprd.  A request is passed to pprd by
writing it into a file and then sending a message over pprd's FIFO.  The
response is received through another file.

=== Installation ===

To enable the IPP server, add this line to /etc/inetd.conf:

 # PPR's IPP server
 ipp stream tcp nowait.400 pprwww /usr/sbin/tcpd /usr/lib/ppr/ppr-httpd --ipp

Disable any other IPP server which may be running (such as CUPS), and then run:

 # kill -HUP _inetd_pid_

