#! /bin/sh
#
# mouse:~ppr/src/libscript/libppr_int.sh
# Copyright 1995--2000, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is"
# without express or implied warranty.
#
# Last modified 21 November 2000.
#

#
# This is an Sh port of ../libppr_int/int_exit.c:int_exit().
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

