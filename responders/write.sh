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
# Last modified 25 March 2005.
#

#
# This responder attempts to send the message with the write
# program.	If that fails, it invokes the mail responder.
#

while [ $# -gt 0 ]
	do
	name=`echo $1 | cut -d= -f1`
	value=`echo $1 | cut -d= -f2`
	case $name in
		for )
			for="$value"
			;;
		responder_address )
			responder_address="$value"
			;;
		short_message )
			short_message="$value"
			;;
	esac
	shift
	done

# Send the message with write.
write $responder_address >/dev/null 2>&1 <<EndOfMessage
To: $for
$short_message
EndOfMessage

# If that didn't work, try the mail responder.
if [ $? -ne 0 ]
	then
	exec @RESPONDERDIR@/mail "$@"
	fi

exit 0
