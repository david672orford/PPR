#! /bin/sh
#
# mouse.trincoll.edu:~ppr/src/misc_filters/xbm.sh
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
XBMTOPBM="?"
PNMTOPS="?"

# Assign names to the parameters
OPTIONS="$1"
PRINTER="$2"
TITLE="$3"

# Set the RESOLUTION option to use the default.
RESOLUTION=""

# Look for parameters we should pay attention to
for pair in $OPTIONS
    do
    case "$pair" in
	resolution=* )
	    RESOLUTION="-dpi `echo $pair | cut -d'=' -f2`"
	    ;;
    esac
    done

$XBMTOPBM | $PNMTOPS $RESOLUTION | grep -v '^%%Title:'

# Exit with the errorlevel left by
# the last program.
exit $?

# end of file
