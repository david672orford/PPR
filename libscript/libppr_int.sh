#! /bin/sh
#
# mouse:~ppr/src/libscript/libppr_int.sh
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
# Last modified 4 April 2003.
#

#
# This is an Sh port of ../libppr_int/int_exit.c:int_exit().  It can be 
# removed a soon as support for the gs* interfaces (which some users
# may still have installed) is removed.
#

. lib/signal.sh

int_exit ()
	{
	if [ "$PPR_GS_INTERFACE_PID" != "" ]
		then
		case $1 in
			$EXIT_PRINTED )
				;;
			$EXIT_PRNERR_NORETRY )
				kill -$SIGUSR2 $PPR_GS_INTERFACE_PID
				;;
			$EXIT_ENGAGED )
				kill -$SIGINT $PPR_GS_INTERFACE_PID
				;;
			* )
				kill -$SIGUSR1 $PPR_GS_INTERFACE_PID
				;;
		esac
		sleep 1
		fi
	exit $1
	}

# end of file

