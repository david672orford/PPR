#! /bin/sh
#
# mouse:~ppr/src/interfaces/dummy.sh
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
# This is a test interface which just copies the input file to the
# output file designated by the printer's address parameter.
#

# source the file which defines the exit codes
. lib/interface.sh

# Detect the --probe option.  We are a fake, so we fake it.
if [ "$1" == "--probe" ]
	then
	echo "PROBE: Product=HP LaserJet 4000 Series"
	exit $EXIT_PRINTED
	fi

# give the parameters names
PRINTER="$1"
ADDRESS="$2"
OPTIONS="$3"
JOBBREAK="$4"
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
if [ "0$FEEDBACK" -ne 0 ]
	then
	lib/alert $PRINTER TRUE "The PPR interface program \"dummy\" is incapable of sending feedback."
	if [ "$PRINTER" != "-" ]
		then
		lib/alert $PRINTER FALSE "Use the command \"ppad feedback $PRINTER false\" to"
		lib/alert $PRINTER FALSE "correct this problem."
		fi
	exit $EXIT_PRNERR_NORETRY_BAD_SETTINGS
	fi

# If the file exists already, make sure we can write to it.	 If it doesn't, 
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
