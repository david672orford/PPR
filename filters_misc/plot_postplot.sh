#! /bin/sh
#
# mouse.trincoll.edu:~ppr/src/misc_filters/plot.sh
# Copyright 1997, 1998, Trinity College Computing Center.
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
# Run postplot from the lp spooling system to convert
# Berkeley plot format files to PostScript.
#
# $1 is the options
# $2 is the name of the printer or group
# $3 is the title
#

POSTPLOT="?"

# Process the options
#for option in $1
#	do
#	case $option in
#		*)
#			;;
#	esac
#	done

# Do the work
$POSTPLOT

# Pass the error code back to our parent
exit $?

# end of file
