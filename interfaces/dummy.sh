#! /bin/sh
#
# mouse:~ppr/src/interfaces/dummy.sh
# Copyright 1995, 1996, 1997, 1998, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software is provided "as is" without express or
# implied warranty.
#
# Last modified 6 November 1998.
#

#
# This is a test interface which just copies the input file to the
# output file designated by the printer's address parameter.
#

# source the file which defines the exit codes
. lib/interface.sh

# give the parameters names
PRINTER="$1"
ADDRESS="$2"
OPTIONS="$3"
FEEDBACK="$5"

SLEEP=""
for opt in $OPTIONS
    do
    case $opt in
	sleep=* )
	    SLEEP=`echo $opt | cut -d'=' -f2`
	    ;;
	* )
	    lib/alert $PRINTER TRUE "Unrecognized interface option: $opt"
	    exit $EXIT_PRNERR_NORETRY
	    ;;
    esac
    done

# Make sure the address parameter is not empty.
if [ -z "$ADDRESS" ]
	then
	lib/alert $PRINTER TRUE "Address is empty"
	exit $EXIT_PRNERR_NORETRY
	fi

# Make sure no one has said we can do feedback.
if [ $FEEDBACK -ne 0 ]
	then
	lib/alert $PRINTER TRUE "This interface doesn't support bidirectional communication."
	lib/alert $PRINTER FALSE "Use the command \"ppad feedback $PRINTER false\" to"
	lib/alert $PRINTER FALSE "correct this problem."
	exit $EXIT_PRNERR_NORETRY
	fi

# copy the file
cat - >$ADDRESS
if [ $? -ne 0 ]
	then
	lib/alert $PRINTER TRUE "cat failed"
	exit $EXIT_PRNERR_NORETRY
	fi

# Sleep if we have been asked to do so
if [ -n "$SLEEP" ]
	then
	sleep $SLEEP
	fi

# say there was no error
exit $EXIT_OK

# end of file


