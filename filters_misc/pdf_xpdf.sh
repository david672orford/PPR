#! /bin/sh
#
# mouse:~ppr/src/filters_misc/pdf_xpdf.sh
# Copyright 1995--2001, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software is provided "as is" without express or
# implied warranty.
#
# Last modified 8 February 2001.
#

#
# This is a PDF filter for the PPR spooling system.
# This version uses the pdf2ps program from xpdf.
#
# $1 is the filter options.
#

HOMEDIR="?"
TEMPDIR="?"
PDFTOPS="?"

# Process the options
LEVEL=""
for option in $1
	do
		case $option in
		level=1)
			LEVEL="-level1"
			;;
		*)
			;;
	esac
	done

tempfile=`$HOMEDIR/lib/mkstemp $TEMPDIR/ppr-pdf-XXXXXX`
cat - >$tempfile

$PDFTOPS $LEVEL $tempfile -
exval=$?

rm -f $tempfile

exit $exval

# end of file

