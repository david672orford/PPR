#! /bin/sh
#
# mouse:~ppr/src/misc/ppr-sync.sh
# Copyright 1995--2005, Trinity College Computing Center.
# Written by David Chappell.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 
# * Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
# 
# * Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE 
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
# POSSIBILITY OF SUCH DAMAGE.
#
# Last modified 6 April 2005.
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
	$RSH $DESTSYS "$HOMEDIR/bin/papsrv --stop; $HOMEDIR/bin/papsrv"
	echo
	fi

# remove the list which we no longer need
rm -f $tempfile

exit 0
