#! /bin/sh
#
# mouse:~ppr/src/misc/ppr-xfm.sh
# Copyright 1995--1999, Trinity College Computing Center.
# Written by David Chappell
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software is provided "as is" without express or
# implied warranty.
#
# Last modified 4 June 1999.
#

#
# This shell script is intended to be invoked from an X-Windows file
# manager.  The first parameter is the name of the printer to print on,
# the second and subsequent parameters are the names of the files to print.
#
# If ppr reports an error, the entire output of ppr is displayed using
# xmessage if it is available and xterm, sh, and echo if it is not.
#

# Save the printer name in a variable sinche we
# will use shift which will destroy the origional $1.
PRINTER="$1"

# If we do not have at least two parameters,
# then we are a queue display program.
if [ -z "$PRINTER" -o -z "$2" ]
    then
    xterm -title "$1's queue" \
	-e /bin/sh -c "while true
		do
		clear
		ppop status $1
		echo
		ppop list $1
		echo
		echo \"Please press control-C when you want to remove this window.\"
		sleep 5
		done"
    exit 0
    fi

# In this loop, we process each file.
while [ -n "$2" ]
    do
    # Run ppr on this file, capturing the output.
    OUTPUT=`ppr -d $PRINTER -C "$2" -m xwin -r "$DISPLAY" -w log "$2" 2>&1`;

    # If there was an error, print it using xmessage if we have it,
    # if not, pass a shell script to xterm.
    if [ $? -ne 0 ]
	then
	if [ -x /usr/bin/X11/xmessage ]
	    then
	    echo "$OUTPUT" \
		| xmessage -geometry +100+100 \
			-title "Printing Errors For \"$2\""\
			-default okay -file -
	    else
	    xterm \
		-geometry 80x10+100+100 -sb \
		-title "Printing Errors for \"$2\"" \
		-e /bin/sh -c "echo \"$OUTPUT\"; echo; echo 'Please press RETURN to clear this message.'; read x"
	    fi
	fi
    shift 1	# shift to the next file
    done

exit 0
