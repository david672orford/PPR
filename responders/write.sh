#! /bin/sh
#
# mouse:~ppr/src/responders/write.sh
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
# This responder attempts to send the message with the write
# program.  If that fails, it invokes the mail responder.
#

# Pull in definitions of $RESP_*:
. lib/respond.sh

# Name those arguments we intend to use:
for="$1"
addr="$2"
msg="$3"
msg2="$4"
responder_options="$5"
response_code_number="$6"

# Parse the responder options
option_printed=1
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

# If invoked with the printed=no option, bail out
# if we were about to report that a job has been printed:
if [ $option_printed -eq 0 ]
    then
    if [ $response_code_number -eq $RESP_FINISHED ]
	then
	exit 0
	fi
    fi

# If invoked with the canceled=no option, bail out if canceled.
if [ $option_canceled -eq 0 ]
    then
    if [ $response_code_number -eq $RESP_CANCELED_PRINTING ]
	then
	exit 0
	fi
    if [ $response_code_number -eq $RESP_CANCELED ]
	then
	exit 0
	fi
    fi

# Send the message with write.
write $addr >/dev/null 2>&1 <<EndOfMessage
To: $for
$msg
EndOfMessage

# If that didn't work, try the mail responder.
if [ $? -ne 0 ]
    then
    exec responders/mail "$@"
    fi

exit 0
