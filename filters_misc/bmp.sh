#! /bin/sh
#
# mouse.trincoll.edu:~ppr/src/misc_filters/bmp.sh
# Copyright 1995, 1997, 1998, Trinity College Computing Center
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software is provided "as is" without express or
# implied warranty.
#
# Last modified 4 September 1998.
#

#
# This program invokes appropriate filters to convert MS-Windows
# BMP files to PostScript.
#

# Paths of filters
BMPTOPPM="?"
PPMTOPGM="?"
PNMTOPS="?"

# Assign names to the parameters
OPTIONS="$1"
PRINTER="$2"
TITLE="$3"

# Set colour option to convert image to grayscale
colour=""

# Look for parameters we should pay attention to
for pair in $OPTIONS
	do
	case "$pair" in
	colour=[yY]* )
		colour="YES"
		;;
	colour=[nN]* )
		colour=""
		;;
	color=[yY]* )
		colour="YES"
		;;
	color=[nN]* )
		colour=""
		;;
	esac
	done

# Run the filters
if [ -n "$colour" ]
	then
	$BMPTOPPM | $PNMTOPS | grep -v '^%%Title:'
	else
	$BMPTOPPM | $PPMTOPGM | $PNMTOPS | grep -v '^%%Title:'
	fi

# Exit with the exit value of the
# last program to be run.
exit $?

# end of file
