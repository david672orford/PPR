#! /bin/sh
#
# mouse:~ppr/src/responders/xwin.sh
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
# Last modified 3 April 2001.
#

#
# This responder sends the response by means of the X-Windows program
# "xmessage" or, if xmessage is not available, by means of xterm, sh,
# and echo.
#
# If you don't have xmessage but do have Tcl/Tk, you can get the xmessage
# clone out of the misc directory of the PPR source code.
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
Probable cause:  $why_arrested
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
when=`lib/time_elapsed $time_submitted 600`
if [ -n "$when" ]
    then
    message="${message}
You submitted this job $when ago.
"
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
