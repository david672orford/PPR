#! /bin/sh
#
# mouse:~ppr/src/commentators/xwin.sh
# Copyright 1995--2003, Trinity College Computing Center.
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
# Last modified 5 April 2003.
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
   echo "    $printer $code $cooked $raw1 $raw2"
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
