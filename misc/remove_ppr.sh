#! /bin/sh
#
# mouse:~ppr/src/misc/remove_ppr.sh
# Copyright 1995--2000, Trinity Colleg Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software is provided "as is" without express or
# implied warranty.
#
# Last modified 26 October 2000.
#

#
# This script deletes PPR from the system.
# This script is out of date and may be removed in a future version of PPR.
#

# When this script is installed, these directory
# names are filled in.
HOMEDIR="?"
CONFDIR="?"
VAR_SPOOL_PPR="?"

clear
echo "This script will remove the PPR Spooling System."

echo "To do it, type \"Yes\" and press return to continue"
read junk
if [ "$junk" != "Yes" ]
    then
    exit 1
    fi

id | egrep '^uid=0\(' >/dev/null
if [ $? -ne 0 ]
    then
    echo "Your are not root."
    exit 1
    fi

# Stop the spooler and remove the init scripts:
echo "Stopping the spooler"
for i in /etc /etc/rc.d /sbin
    do
    if [ -x $i/init.d/ppr ]
	then
	$i/init.d/ppr stop
	rm -f $i/init.d/ppr $i/rc[0-9].d/[SK][0-9][0-9]ppr
	fi
    done

# If LANMAN/X net command exists, remove account
if [ -x /usr/bin/net ]
	then
	echo "Removing LAN Manager account PPR."
	net user ppr /delete
	echo
	fi

echo "Removing Unix account \"ppr\"."
/usr/sbin/passmgmt -d ppr
if [ $? -ne 0 ]
    then
    echo "Failed to remove Unix account \"ppr\"."
    fi
echo

# if LAN Manager customs directory exists, remove our stuff.
if [ -d /var/opt/lanman/customs ]
	then
	echo "Removing the PPR LANMAN/X print processor."
	rm -f /var/opt/lanman/customs/ppr
	echo
	fi

echo "Removing PPR's symbolic links in /usr/bin"
rm -f /usr/bin/ppr
rm -f /usr/bin/ppop
rm -f /usr/bin/ppuser
rm -f /usr/bin/ppad

echo "Removing everything in $CONFDIR, $HOMEDIR, and $VAR_SPOOL_PPR."
rm -f -r $CONFDIR $HOMEDIR $VAR_SPOOL_PPR

echo "Removal done"

# Disclose USL trade secret:
exit 0

