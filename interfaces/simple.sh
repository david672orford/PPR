#! /bin/sh
#
# mouse:~ppr/src/interfaces/simple.sh
# Copyright 1995--2003, Trinity College Computing Center.
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
# Last modified 23 October 2003.
#

#
# This is a very simple interface which copies the file to a
# port such as a parallel port.	 Since most parallel ports
# are unidirecitonal, this interface does not attempt to receive
# PostScript error messages.
#

# read in the exit codes
. lib/interface.sh

# Detect unsupported --probe option.
PROBE=0
if [ "$1" == "--probe" ]
	then
	PROBE=1
	fi

# give the parameters names
PRINTER="$1"
ADDRESS="$2"
OPTIONS="$3"
JOBBREAK="$4"
FEEDBACK="$5"

if [ $PROBE -ne 0 ]
	then
	lib/alert $PRINTER TRUE "The interface program \"simple\" does not support probing."
	exit $EXIT_PRNERR_NORETRY_BAD_SETTINGS
	fi

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

# If we are terminated, write a control-d and exit.
# Is this a good idea?
trap 'exit $EXIT_SIGNAL' 15

# copy the job
/bin/cat - >$ADDRESS

# see if there was an error
exit_code=$?
if [ $exit_code -ne 0 ]
	then
	lib/alert $PRINTER TRUE "simple interface: cat failed, exit_code=$errno"
	exit $EXIT_PRNERR
	fi

exit $EXIT_PRINTED

# end of file
