#! @SHELL@
#
# mouse:~ppr/src/responders/xwin.sh
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
# This responder sends the response by means of the X-Windows program 
# "xmessage" or, if xmessage is not available, by means of wish and our
# xmessage clone script, or if that isn't available either, by means of 
# xterm, sh, and echo.
#

#==============================
# Parse the command line
#==============================
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

echo $short_message >&2

#==============================
# Parse the responder options
#==============================
option_timeout=""
for opt in $responder_options
	do
	case $opt in
		timeout=* )
			option_timeout="-timeout `echo $opt | cut -d= -f2`"
			;;
		esac
	done

#===========================================================
# Figure out which program we can use so send the message.
#===========================================================

if [ -x @XWINBINDIR@/xmessage ]
	then
	sender="@XWINBINDIR@/xmessage \
		-geometry +100+100 \
		-default okay $option_timeout \
		-file -
"

	elif [ -x /usr/local/bin/wish ]
	then
	sender="/usr/local/bin/wish @LIBDIR@/xmessage \
		-geometry +100+100 \
		-default okay $option_timeout \
		-file -
"

	elif [ -x /usr/bin/wish ]
	then
	sender="/usr/bin/wish $LIBDIR/xmessage \
		-geometry +100+100 \
		-default okay $option_timeout \
		-file -
"

	elif [ -x @XWINBINDIR@/rxvt ]
	then
	sender="@XWINBINDIR@/rxvt \
	-geometry 80x5+100+100 \
	-e /bin/sh -c 'cat; echo \"Press ENTER to dismiss this message.\"; read x'
"

	elif [ -x @XWINBINDIR@/xterm ]
	then
	sender="@XWINBINDIR@/xterm \
	-geometry 80x5+100+100 \
	-e /bin/sh -c 'cat; echo \"Press ENTER to dismiss this message.\"; read x'
"

	else
	echo "Can't find a program to respond with!"
	fi

#====================================
# Dispatch the message.
#====================================

$sender -display "$responder_address" \
	-title "Message for $for" \
	-bg skyblue -fg black \
	<<EndOfMessage
$short_message
EndOfMessage

exit 0
