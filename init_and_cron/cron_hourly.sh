#! /bin/sh
#
# mouse:~ppr/src/init_and_cron/cron_hourly.sh
# Copyright 1995--2000, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appears in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is"
# without express or implied warranty.
#
# Last modified 16 November 2000.
#

HOMEDIR="?"
VAR_SPOOL_PPR="?"

# This is a list of files which might be lists of installed packages.  The
# file /var/lib/rpm/packages.rpm is for Red Hat's RPM format, the file
# /var/sadm/install/contents is for Solaris's package format.
PACKAGE_LISTS="/var/lib/rpm/packages.rpm /var/sadm/install/contents"

#
# If any of the package lists has changed since the font index was
# generated, rebuild the font index now.
#
for i in $PACKAGE_LISTS
    do
    if [ -f $i ]
	then
	if $HOMEDIR/lib/file_outdated $VAR_SPOOL_PPR/fontindex.db $i
	    then
	    $HOMEDIR/bin/ppr-indexfonts
	    break
	    fi
	fi
    done

exit 0
