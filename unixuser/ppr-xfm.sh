#! /bin/sh
#
# mouse:~ppr/src/misc/ppr-xfm.sh
# Copyright 1995--2003, Trinity College Computing Center.
# Written by David Chappell
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
# Last modified 13 March 2003.
#

#
# This shell script is intended to be invoked from an X-Windows file
# manager.	The first parameter is the name of the printer to print on,
# the second and subsequent parameters are the names of the files to print.
#
# If ppr reports an error, the entire output of ppr is displayed using
# xmessage if it is available and xterm, sh, and echo if it is not.
#

# Save the printer name in a variable since we
# will use shift which will destroy the origional $1.
PRINTER="$1"
shift

# If we do not have at least two parameters,
# then we are a queue display program.
if [ -z "$1" ]
	then
	if [ -z "$PRINTER" ]
	then
	PRINTER="all"
	fi
	xterm -title "$PRINTER's queue" \
	-e /bin/sh -c "while true
		do
		clear
		ppop status $PRINTER
		echo
		ppop list $PRINTER
		echo
		echo \"Please press control-C when you want to remove this window.\"
		sleep 5
		done"
	exit 0
	fi

# In this loop, we process each file.
while [ -n "$1" ]
	do
	# Run ppr on this file, capturing the output.
	OUTPUT=`ppr -d $PRINTER -C "$1" -m xwin -r "$DISPLAY" -w log "$1" 2>&1`;

	# If there was an error, print it using xmessage if we have it,
	# if not, pass a shell script to xterm.
	if [ $? -ne 0 ]
	then
	if [ -x /usr/bin/X11/xmessage ]
		then
		echo "$OUTPUT" \
		| xmessage -geometry +100+100 \
			-title "Printing Errors For \"$1\""\
			-default okay -file -
		else
		xterm \
		-geometry 80x10+100+100 -sb \
		-title "Printing Errors for \"$1\"" \
		-e /bin/sh -c "echo \"$OUTPUT\"; echo; echo 'Please press RETURN to clear this message.'; read x"
		fi
	fi
	shift 1 # shift to the next file
	done

exit 0
