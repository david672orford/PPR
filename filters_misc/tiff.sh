#! /bin/sh
#
# mouse.trincoll.edu:~ppr/src/misc_filters/tiff.sh
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

# Paths of filters
TIFFTOPNM="?"
PPMTOPGM="?"
PNMDEPTH="?"
PNMTOPS="?"

# Assign names to the parameters
OPTIONS="$1"
PRINTER="$2"
TITLE="$3"

# Set COLOUR option to convert image to grayscale.
# Set the RESOLUTION option to use the default.
COLOUR=""
RESOLUTION=""

# Look for parameters we should pay attention to
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
	$TIFFTOPNM | $PNMDEPTH 255 | $PNMTOPS $RESOLUTION | grep -v '^%%Title:'
	else
	$TIFFTOPNM | $PPMTOPGM | $PNMDEPTH 255 | $PNMTOPS $RESOLUTION | grep -v '^%%Title:'
	fi

# Exit with the errorlevel left by
# the last program.
exit $?

# end of file
