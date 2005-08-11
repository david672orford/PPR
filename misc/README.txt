mouse:~ppr/src/misc/README.txt
17 January 2005

The PPR source directory "misc/" contains a number of small programs and other
files.  Some of these are not described in the primary documentation.  Some
are not installed automatically.  Many of them are of questionable value.


=================================
 ppd2macosdrv.perl
=================================

This program converts the PPD files to MacOS format and installs them in
$VAR_SPOOL_PPR/drivers/macos.  Presumably, the system administrator will
then arrange to share that directory with CAP or Netatalk.


=================================
 ppr-sync.sh
=================================

This shell script can be used to syncronize the printer configurations on
two computers running PPR.  After making a change on one computer, while
logged in as "ppr", run "ppr-sync <othercomputer>".  This script uses rsh to
transfer the files.  It is really just a hack and not a very good one at that.


===================================
 xmessage.tcl
===================================

This is a Tcl/Tk wish script which is intended as a substitute for xmessage.

The program xmessage is part of some X distributions.  It is a simple utility
which is handy for popuping up simple dialog boxes from shell scripts.
Sadly, many X-Windows distributions don't include this neat little program.
This is not a perfect clone, but it is close enough to work for PPR.

This program is installed in $LIBDIR.  The xwin responder and xwin
commentator will use it if xmessage is not found and wish is.


====================================
 custom_hook_docutech.perl
====================================

This script is for use with a Xerox Docutech 135 in Trinity College's print
shop.  Use it as an example.  It is installed in $LIBDIR.


====================================
 ppr-wp-1.12.zip
====================================

This file contains WordPerfect 5.1 PostScript drivers for the HP LaserJet 4M
and a generic PostScript level 1 printer which work much better with PPR than
the ones supplied by WordPerfect Corp.


====================================
End of file
