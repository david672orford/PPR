#! /bin/sh
#
# mouse:~ppr/src/fixup/fixup_cron.sh
# Copyright 1995--2000, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is"
# without express or implied warranty.
#
# Last modified 18 November 2000.
#

HOMEDIR="?"
VAR_SPOOL_PPR="?"
TEMPDIR="?"
USER_PPR=?

echo "Creating a crontab for the user $USER_PPR ..."

id | egrep "^uid=[0-9]+\($USER_PPR\)" >/dev/null
if [ $? -ne 0 ]
    then
    echo "This script must be run as the user \"$USER_PPR\"!"
    exit 1
    fi

# Get the contents of the current crontab.
#current=`crontab -l 2>/dev/null`
#if [ "$current" != "" ]
#    then
#    echo "There already is a crontab for $USER_PPR."
#    echo
#    exit 1
#    fi

# Create a temporary file.
tempname=`$HOMEDIR/lib/mkstemp $TEMPDIR/ppr-fixup_cron-XXXXXX`

# Write the new crontab to a temporary file.
cat - >$tempname <<===EndOfHere===
3 10,16 * * 1-5 $HOMEDIR/bin/ppad remind
5 4 * * * $HOMEDIR/lib/cron_daily
17 * * * * $HOMEDIR/lib/cron_hourly
===EndOfHere===

# Sadly, not all versions of crontab can read from stdin, at least not
# with the same command.
crontab $tempname

rm $tempname

echo "Done."
echo

exit 0

