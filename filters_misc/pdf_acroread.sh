#! /bin/sh
#
# mouse:~ppr/src/filters_misc/pdf_acroread.sh
# Copyright 1995--1999, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software is provided "as is" without express or
# implied warranty.
#
# Last modified 9 July 1999.
#

#
# This is a PDF filter for the PPR spooling system.
# This version uses the free Adobe Acrobat Reader.
#
# $1 is the filter options.
#

ACROREAD="?"

# Process the options
LEVEL=""
for option in $1
	do
	case $option in
		level=2)
			LEVEL="-level2"
			;;
		*)
			;;
	esac
	done

$ACROREAD -toPostScript $LEVEL

exit $?

# end of file

