#! /bin/sh
#
# mouse:~ppr/src/interfaces/simple.sh
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
# Last modified 11 May 2001.
#

#
# This is a very simple interface which copies the file to a
# port such as a parallel port.  Since most parallel ports
# are unidirecitonal, this interface does not attempt to receive
# PostScript error messages.
#

PRINTER="$1"
ADDRESS="$2"
OPTIONS="$3"
FEEDBACK="$5"

# read in the exit codes
. lib/interface.sh

# make sure the port can be written to
if [ ! -w "$ADDRESS" ]
	then
	if [ -c "$ADDRESS" ]
		then
		lib/alert $PRINTER TRUE "No permission to open port \"$ADDRESS\"."
		exit $EXIT_PRNERR_NORETRY_ACCESS_DENIED
		else
		lib/alert $PRINTER TRUE "Port \"$ADDRESS\" does not exist."
		exit $EXIT_PRNERR_NORETRY_NO_SUCH_ADDRESS
		fi
	fi

# Make sure no one has said we can do feedback.
if [ $FEEDBACK -ne 0 ]
	then
	lib/alert $PRINTER TRUE "This interface doesn't support bidirectional communication."
	lib/alert $PRINTER FALSE "Use the command \"ppad feedback $PRINTER false\" to"
	lib/alert $PRINTER FALSE "correct this problem."
	exit $EXIT_PRNERR_NORETRY_BAD_SETTINGS
	fi

# change standard output to the port
exec >$ADDRESS

# If we are terminated, write a control-d and exit.
# Is this a good idea?
trap 'echo "\04"; exit $EXIT_SIGNAL' 15

# copy the job
/bin/cat -

# see if there was an error
exit_code=$?
if [ $exit_code -ne 0 ]
	then
	lib/alert $PRINTER TRUE "simple interface: cat failed, exit_code=$errno"
	exit $EXIT_PRNERR
	fi

exit $EXIT_PRINTED

# end of file
