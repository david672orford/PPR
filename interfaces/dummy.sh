#! /bin/sh
#
# mouse:~ppr/src/interfaces/dummy.sh
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
# Last modified 13 November 2001.
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

# Parse the options.
SLEEP=""
CREATE=1
for opt in $OPTIONS
    do
    case $opt in
	sleep=* )
	    SLEEP=`echo $opt | cut -d'=' -f2`
	    ;;
	create=[yYtT1]* )
	    CREATE=1
	    ;;
	create=[nNfF0]* )
	    CREATE=0
	    ;;
	* )
	    lib/alert $PRINTER TRUE "Unrecognized interface option: $opt"
	    exit $EXIT_PRNERR_NORETRY_BAD_SETTINGS
	    ;;
    esac
    done

# Make sure the address parameter is not empty.
if [ -z "$ADDRESS" ]
	then
	lib/alert $PRINTER TRUE "Address is empty"
	exit $EXIT_PRNERR_NORETRY_BAD_SETTINGS
	fi

# Make sure no one has said we can do feedback.
if [ $FEEDBACK -ne 0 ]
	then
	lib/alert $PRINTER TRUE "This interface doesn't support bidirectional communication."
	lib/alert $PRINTER FALSE "Use the command \"ppad feedback $PRINTER false\" to"
	lib/alert $PRINTER FALSE "correct this problem."
	exit $EXIT_PRNERR_NORETRY_BAD_SETTINGS
	fi

# If the file exists already, make sure we can write to it.  If it doesn't, 
# make sure that that is ok.
if [ -f $ADDRESS ]
    then
    if [ ! -w $ADDRESS ]
	then
	lib/alert $PRINTER TRUE "Access to the file or character device \"$ADDRESS\" is denied."
	exit $EXIT_PRNERR_NORETRY_ACCESS_DENIED
	fi
    else
    if [ $CREATE -eq 0 ]
	then
	lib/alert $PRINTER TRUE "There is no file or character device \"$ADDRESS\"."
	exit $EXIT_PRNERR_NORETRY_NO_SUCH_ADDRESS
	fi
    fi

# copy the file
/bin/cat - >$ADDRESS
if [ $? -ne 0 ]
	then
	lib/alert $PRINTER TRUE "dummy interface: cat failed"
	exit $EXIT_PRNERR
	fi

# Sleep if we have been asked to do so
if [ -n "$SLEEP" ]
	then
	sleep $SLEEP
	fi

# say there was no error
exit $EXIT_OK

# end of file
