#! @SHELL@
#
# mouse:~ppr/src/interfaces/simple.sh
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
# This is a very simple printer interface program.  All it does is copy the
# file to the port whole filename is specified by the address.  Since there
# is no provision for setting things such as baud rate, the port should
# either have suitable default values or be a port without important
# settings, such as a parallel port.  This interface does not attempt to
# support bidirectional communication.
#

# read in the exit codes
. lib/interface.sh

# We are too simple to probe.  If asked too, we will print an error message
# on stderr and stop with a thud.
if [ "$1" == "--probe" ]
	then
	echo "The interface program \"`basename $0`\" does not support probing." >&2
	exit $EXIT_PRNERR_NORETRY_BAD_SETTINGS
	fi

# Assign names to the parameters.
PRINTER="$1"
ADDRESS="$2"
OPTIONS="$3"
JOBBREAK="$4"
FEEDBACK="$5"

# Make sure the port is writable.  If it isn't operator intervention will
# be required, so we use exit codes that stop the queue and leave it stopt.
if [ ! -w "$ADDRESS" ]
	then
	# It isn't writable.  Is there such a character device?
	if [ -c "$ADDRESS" ]
		then
		lib/alert $PRINTER TRUE "No permission to open port \"$ADDRESS\"."
		exit $EXIT_PRNERR_NORETRY_ACCESS_DENIED
		else
		lib/alert $PRINTER TRUE "Port \"$ADDRESS\" does not exist."
		exit $EXIT_PRNERR_NORETRY_NO_SUCH_ADDRESS
		fi
	fi

# Make sure no one has said we can do feedback.  If pprdrv has asked us to,
# howl and exit with a code that stops the queue and leaves it stopt.
if [ $FEEDBACK -ne 0 ]
	then
	lib/alert $PRINTER TRUE "This interface doesn't support bidirectional communication."
	lib/alert $PRINTER FALSE "Use the command \"ppad feedback $PRINTER false\" to"
	lib/alert $PRINTER FALSE "correct this problem."
	exit $EXIT_PRNERR_NORETRY_BAD_SETTINGS
	fi

# This is it.  Copy that job!
/bin/cat - >$ADDRESS

# Did it work?  If it didn't, log an alert.  Since we don't really know _why_
# it failed, we exit with a code that allows pprdrv to try again in a little
# while.
exit_code=$?
if [ $exit_code -ne 0 ]
	then
	lib/alert $PRINTER TRUE "simple interface: cat failed, exit_code=$errno"
	exit $EXIT_PRNERR
	fi

# Inform pprdrv that we have done our part to completion.
exit $EXIT_PRINTED

# end of file
