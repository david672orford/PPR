#! /bin/sh
#
# mouse:~ppr/src/misc/ppr-sync.sh
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
# Last modified 16 February 2001.
#

#
# Copy all of a computer's PPR configuration to another computer.
#

# Use a fairly safe PATH
PATH="/usr/local/bin:/usr/bin:/bin"
export PATH

# The directories in which we will work.
HOMEDIR="?"
CONFDIR="?"
TEMPDIR="?"

# Determine the user name of the current user.
MYUID=`id | sed -ne 's/^uid=[0-9][0-9]*(\([^)]*\)).*$/\1/p'`

if [ -z "$MYUID" ]
	then
	echo 'Internal error: Failed to determine user id'
	exit 1
	fi

# if we are root, become ppr
case "$MYUID" in
	root )
		exec su ppr -c "$0 $1"
		;;
	ppr )
		;;
	* )
		echo "You must be \"ppr\" or \"root\" to run this program."
		exit 1
		;;
	esac

# establish that we have a destination system name
DESTSYS="$1"

if [ -z "$DESTSYS" ]
	then
	echo "Usage: pprsync _destination_system_"
	exit 1
	fi

#
# Figure out if we should use rsh or ssh.
#
if [ -d "$HOMEDIR/.ssh" ]
    then
    RSH="ssh"
    RCP="scp"
    else
    RSH="rsh"
    RCP="rcp"
    fi

#
# Remove editor backup files before they cause trouble:
#
rm -f $CONFDIR/printers/*~ $CONFDIR/printers/*.bak
rm -f $CONFDIR/groups/*~ $CONFDIR/groups/*.bak

#
# Create a temporary file.
#
tempfile=`$HOMEDIR/lib/mkstemp $TEMPDIR/ppr-sync-XXXXXX`

#
# Copy the configuration directory to the destination system.
# While we do so, we make a list of those files which were not replaced.
#
echo "Copying configuration files"
find $CONFDIR ! -name 'lw*.conf' ! -name 'papsrv_default_zone.conf' -print \
	| cpio -oc | $RSH $DESTSYS 'cpio -i' >$tempfile 2>&1
echo

# Touch updated printers.
echo "Touching updated printers"
cd $CONFDIR/printers
for printer in *
	do
	egrep "$CONFDIR/printers/$printer" <$tempfile >/dev/null
	if [ $? -ne 0 ]
		then
		echo "    Touching printer \"$printer\""
		$RSH $DESTSYS "$HOMEDIR/bin/ppad touch $printer"
		fi
	done
echo

# touch updated groups
echo "Touching updated groups"
cd $CONFDIR/groups
for group in *
	do
	egrep "$CONFDIR/groups/$group" <$tempfile >/dev/null
	if [ $? -ne 0 ]
		then
		echo "    Touching group \"$group\""
		$RSH $DESTSYS "$HOMEDIR/bin/ppad group touch $group"
		fi
	done
echo

# Restart papsrv if necessary
egrep "$CONFDIR/papsrv" <$tempfile >/dev/null
if [ $? -ne 0 ]
    then
    echo "Restarting papsrv on $DESTSYS"
    $RSH $DESTSYS "$HOMEDIR/bin/papsrv_kill; $HOMEDIR/bin/papsrv"
    echo
    fi

# remove the list which we no longer need
rm -f $tempfile

exit 0
