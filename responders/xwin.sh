#! /bin/sh
#
# mouse:~ppr/src/responders/xwin.sh
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
# Last modified 17 December 2003.
#

#
# This responder sends the response by means of the X-Windows program 
# "xmessage" or, if xmessage is not available, by means of wish and our
# xmessage clone script, or if that isn't available either, by means of 
# xterm, sh, and echo.
#

# Place where the system X-Windows binaries are kept.  It is
# normally "/usr/bin/X11".
XWINBINDIR="?"

# This helps use find our mkstemp.
HOMEDIR="?"

# We may need to write the message into a temporary file.
# It is normally "/tmp".
TEMPDIR="?"

# We need the echo that will accept escape codes.
EECHO="?"

# Give names to all the parameters.
for="$1"
address="$2"
canned_message="$3"
canned_message2="$4"
responder_options="$5"
response_code_number="$6"
jobid="$7"
extra="$8"
title="$9"
shift 9
time_submitted="$1"
why_arrested="$2"
pages_printed="$3"
charge="$4"

# Pull in definitions of $RESP_*:
. lib/respond.sh

#==============================
# Parse the responder options
#==============================
option_printed=1
option_timeout=""
option_canceled=1
for opt in $responder_options
	do
	case $opt in
	printed=[nNfF0]* )
		option_printed=0
		;;
	printed=[yYtT1-9]* )
		option_printed=1
		;;
	timeout=* )
		option_timeout="-timeout `echo $opt | cut -d= -f2`"
		;;
	canceled=[nNfF0]* )
		option_canceled=0
		;;
	canceled=[yYtT1-9]* )
		option_canceled=1
		;;
	* )
		;;
	esac
	done

#==========================================
# Decide if this message is worth sending.
#==========================================

# If the option printed=no appeared and this message mearly
# announces that the job was printed, skip it.
if [ $option_printed -eq 0 ]
  then
  if [ $response_code_number -eq $RESP_FINISHED ]; then exit 0; fi
  fi

# If invoked with the canceled=no option, bail out if canceled.
if [ $option_canceled -eq 0 ]
  then
  if [ $response_code_number -eq $RESP_CANCELED_PRINTING ]; then exit 0; fi
  if [ $response_code_number -eq $RESP_CANCELED ]; then exit 0; fi
  fi

#====================================
# Build the message.
#====================================

# Start with the canned message.
message="$canned_message
";

if [ -n "$why_arrested" ]
	then 
	message="${message}
Probable cause:	 $why_arrested
"
	fi

# Possible add the title.
if [ -n "$title" ]
	then
	message="${message}
The title of this job is \"$title\".
"
	fi

if [ "$pages_printed" != "?" ]		# if printed,
	then
	if [ "$pages_printed" -ne -1 ]	# if number of pages is known,
	then
	if [ "$pages_printed" -eq 1 ]
		then
		message="${message}
It is 1 page long."
		else
		message="${message}
It is $pages_printed pages long."
		fi

	if [ -n "$charge" ]
		then 
		message="${message}  You have been charged $charge."
		fi

	message="${message}
"
	fi
	fi

# If it was submitted more than 10 minutes ago, tell when.
if [ $time_submitted -gt 0 ]
	then
	when=`lib/time_elapsed $time_submitted 600`
	if [ -n "$when" ]
	then
	message="${message}
You submitted this job $when ago.
"
	fi
	fi

#===========================================================
# Figure out which program we can use so send the message.
#===========================================================

if [ -x $XWINBINDIR/xmessage ]
	then
	sender="$XWINBINDIR/xmessage \
		-geometry +100+100 \
		-default okay $option_timeout \
		-file -
"

	else
	if [ -x /usr/local/bin/wish ]
	then
	sender="/usr/local/bin/wish $HOMEDIR/lib/xmessage \
		-geometry +100+100 \
		-default okay $option_timeout \
		-file -
"

	else
	if [ -x /usr/bin/wish ]
	then
	sender="/usr/bin/wish $HOMEDIR/lib/xmessage \
		-geometry +100+100 \
		-default okay $option_timeout \
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

#====================================
# Dispatch the message.
#====================================

$sender -display "$address" \
	-title "Message for $for" \
	-bg skyblue -fg black \
	<<EndOfMessage
$message
EndOfMessage

exit 0
