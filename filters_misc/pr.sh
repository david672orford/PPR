#! /bin/sh
#
# mouse:~ppr/src/misc_filters/pr.sh
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
# Last modified 7 February 2001.
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

HOMEDIR="?"
TEMPDIR="?"
PR="?"

title="$3"
width=""
length=""

tempfile=`$HOMEDIR/lib/mkstemp $TEMPDIR/ppr-pr-XXXXXX`

# Process the options
for option in $1
	do
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

