#! @SHELL@
#
# mouse:~ppr/src/responders/write.sh
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
# Last modified 13 January 2005.
#

#
# This responder attempts to send the message with the write
# program.	If that fails, it invokes the mail responder.
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
