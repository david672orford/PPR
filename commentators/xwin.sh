#! /bin/sh
#
# mouse:~ppr/src/commentators/xwin.sh
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
# Last modified 4 May 2001.
#

#
# This simple commentator pops up an Xterm or Xmessage on the
# X display indicated by the address parameter.	 This commentator
# is intended as a demonstratation.	 It does not attempt to produce
# well formatted or attractive messages.
#

# This are filled in by scriptfixup.sh.
XWINBINDIR="?"
HOMEDIR="?"

# Give the perameters names.
address="$1"
options="$2"
printer="$3"
code="$4"
cooked="$5"
raw1="$6"
raw2="$7"
severity="$8"
canned_message="$9"

# Parse the options.
severity_threshold=3
for opt in $options
   do
   case $opt in
	severity=* )
		severity_threshold=`echo $opt | cut -d'=' -f2`
		;;
   esac
   done

# If this message isn't severe enough, bail out.
if [ $severity -lt $severity_threshold ]
   then
   echo "xwin commentator: not important enough to send:"
   echo "	 $printer $code $cooked $raw1 $raw2"
   exit 0
   fi

#
# Figure out which program we can use so send the message.
#
if [ -x $XWINBINDIR/xmessage ]
	then
	sender="$XWINBINDIR/xmessage \
		-geometry +100+100 \
		-default okay -timeout 60 \
		-file -
"

	else
	if [ -x /usr/local/bin/wish ]
	then
	sender="/usr/local/bin/wish $HOMEDIR/lib/xmessage \
		-geometry +100+100 \
		-default okay -timeout 60 \
		-file -
"

	else
	if [ -x /usr/bin/wish ]
	then
	sender="/usr/bin/wish $HOMEDIR/lib/xmessage \
		-geometry +100+100 \
		-default okay -timeout 60 \
		-file -
"

	else
	if [ -x $XWINBINDIR/rxvt ]
	then
	sender="$XWINBINDIR/rxvt \
	-geometry 80x5+100+100 \
	-e /bin/sh -c 'cat; echo \"Press ENTER to dismiss this message.\"; read x'
"

	else
	if [ -x $XWINBINDIR/xterm ]
	then
	sender="$XWINBINDIR/xterm \
	-geometry 80x5+100+100 \
	-e /bin/sh -c 'cat; echo \"Press ENTER to dismiss this message.\"; read x'
"

	else
	echo "Can't find a program to respond with!"
	fi
	fi
	fi
	fi
	fi

#
# Send the message.
#
$sender -display "$address" \
	-title "Operator message concerning \"$printer\" (`uname -n`)." \
	-bg pink -fg black \
	<<EndOfMessage
$printer: $cooked

$canned_message
EndOfMessage

exit 0
