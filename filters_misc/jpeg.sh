#! /bin/sh
#
# mouse:~ppr/src/filters_misc/jpeg.sh
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
# Last modified 7 July 1999.
#

#
# This program invokes appropriate filters to convert JPEG
# files to PostScript.
#
# The program "~ppr/install/setup_filters" passes this program
# thru a sed script before installing it.
#

# Paths of filters
DJPEG="?"
PNMTOPS="?"

# Assign names to the parameters
OPTIONS="$1"
PRINTER="$2"
TITLE="$3"

# Set colour option to convert image to grayscale
COLOUR="-grayscale"
RESOLUTION=""

# Look for parameters we should pay attention to
for pair in $OPTIONS
    do
    case "$pair" in
	colour=[yYtT1]* )
	    COLOUR=""
	    ;;
	colour=[nNfF0]* )
	    colour="-grayscale"
	    ;;
	color=[yYtT1]* )
	    colour=""
	    ;;
	color=[nNfF0]* )
	    colour="-grayscale"
	    ;;
	resolution=* )
	    RESOLUTION="-dpi `echo $pair | cut -d'=' -f2`"
	    ;;
    esac
    done

# Run the filters
$DJPEG $COLOUR | $PNMTOPS $RESOLUTION | grep -v '^%%Title:'

# Pass on the exit value of the filter pipeline.
exit $?

# end of file
