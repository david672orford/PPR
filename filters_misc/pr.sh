#! /bin/sh
#
# mouse:~ppr/src/misc_filters/pr.sh
# Copyright 1995--2002, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software is provided "as is" without express or
# implied warranty.
#
# Last modified 12 November 2002.
#

#
# PR filter for the PPR spooling system.
# Combine pr and filter_lp to make a PostScript pr.
#
# $1 is the options
# $2 is the name of the printer or group
# $3 is the title
# $4 is the directory ppr was invoked in
#

# These are filled in when the script is installed.
HOMEDIR="/usr/lib/ppr"
TEMPDIR="/tmp"
PR="/usr/bin/pr"

# Process the options
options="a=1 title=\"hello world\" c=3"
#options=`echo $1 | sed -e 's/title="/"title=/'`
title="$3"
width=""
length=""
for option in "$options"
	do
	echo "option: $option" >&2
	case $option in
		width=*)
			width="-w `echo $option | cut -f2 -d'='`"
			;;
		length=*)
			length="-l `echo $option | cut -f2 -d'='`"
			;;
		title=*)
			title=`echo $option | cut -f2 -d'='`
			;;
		*)
			;;
	esac
	done

# Create a temporary file.
tempfile=`$HOMEDIR/lib/mkstemp $TEMPDIR/ppr-pr-XXXXXX`

# Now, run pr.
$PR -f -h "$title" $width $length >$tempfile
if [ $? != 0 ]; then rm -f $tempfile; exit 1; fi

# Now, run filter_lp on the temporary file.
filters/filter_lp "$1" <$tempfile
exval=$?

# We can remove the temporary file now.
rm -f $tempfile

# Make non-zero exit if filter_lp did.
if [ $exval != 0 ]; then exit 2; fi

# We are done, we were sucessfull.
exit 0

