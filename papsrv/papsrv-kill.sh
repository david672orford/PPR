#! /bin/sh
#
# mouse:~ppr/src/papsrv/papsrv-kill.sh
# Copyright 1995--1999, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is" without
# express or implied warranty.
#
# Last modified 5 December 2001.
#

# Filled in by scriptfixup.sh:
HOMEDIR="?"
VAR_SPOOL_PPR="?"

RUNDIR="$VAR_SPOOL_PPR/run"

if [ ! -f $RUNDIR/papsrv.pid ]
	then
	echo "papsrv-kill: papsrv is not running"
	exit 1
	fi

pid=`cat $RUNDIR/papsrv.pid`

echo "Killing papsrv, PID=${pid}."
kill $pid || exit 1

while [ -f $RUNDIR/papsrv.pid ]
	do
	echo "Waiting while papsrv shuts down..."
	sleep 1
	done

echo "Shutdown complete."

exit 0

