#! /bin/sh
#
# mouse.trincoll.edu:~ppr/src/misc_filters/gif.sh
# Copyright 1995, 1997, 1998, Trinity College Computing Center
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
# This program invokes appropriate filters to convert GIF
# files to PostScript.
#
# $HOMEDIR/install/setup_filters passes this program thru a sed
# script before installing it.
#

# Paths of filters
GIFTOPNM="?"
PPMTOPGM="?"
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
	$GIFTOPNM | $PNMTOPS $RESOLUTION | grep -v '^%%Title:'
	else
	$GIFTOPNM | $PPMTOPGM | $PNMTOPS $RESOLUTION | grep -v '^%%Title:'
	fi

# Pass on the exit value of the filter pipeline.
exit $?

# end of file
