#! /bin/sh
#
# mouse:~ppr/src/misc_filters/pnm.sh
# Copyright 1995--1999, Trinity College Computing Center
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software is provided "as is" without express or
# implied warranty.
#
# Last modified 28 June 1999.
#

# Paths of filters.  These are filled
# in when the filter is installed.
PNGTOPNM="?"
PPMTOPGM="?"
PNMDEPTH="?"
PNMTOPS="?"

# Assign names to the parameters.
OPTIONS="$1"
PRINTER="$2"
TITLE="$3"

# Set COLOUR option to convert image to grayscale.
# Set the RESOLUTION option to use the default.
COLOUR=""
RESOLUTION=""

# Look for parameters we should pay attention to.
for pair in $OPTIONS
    do
    case "$pair" in
	colour=[yYtT1]* )
	    COLOUR="YES"
	    ;;
	colour=[nNfF0]* )
	    COLOUR=""
	    ;;
	color=[yYtT1]* )
	    COLOUR="YES"
	    ;;
	color=[nNfF0]* )
	    COLOUR=""
	    ;;
	resolution=* )
	    RESOLUTION="-dpi `echo $pair | cut -d'=' -f2`"
	    ;;
    esac
    done

# Run the filters
if [ -n "$COLOUR" ]
	then
	$PNGTOPNM | $PNMDEPTH 255 | $PNMTOPS $RESOLUTION | grep -v '^%%Title:'
	else
	$PNGTOPNM | $PPMTOPGM | $PNMDEPTH 255 | $PNMTOPS $RESOLUTION | grep -v '^%%Title:'
	fi

# Pass on the exit value of the filter pipeline.
exit $?

# end of file
