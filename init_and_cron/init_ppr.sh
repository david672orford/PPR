#! /bin/sh
#
# mouse:~ppr/src/init_and_cron/init_ppr.sh
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
# Last modified 13 February 2001.
#

#
# This file is designed to work with the System V init provided with
# operating systems such as SunOS 5.  If invoked with the argument
# "start", it starts PPRD and PAPSRV, if invoked with the argument
# "stop", it is supposed to stop them.  The "/usr/lib/ppr/fixup/fixup_init"
# script will install this script if necessary.
#

#
# This information in this block is for the user of the Linux program
# /sbin/chkconfig which can be used to install and remove links from
# the init.d directory to the rc?.d directories.
#
# chkconfig: 2345 80 40
# description: PPR is a print spooler for PostScript printers.
#

# System configuration values in this section are filled in when the
# script is installed.
HOMEDIR="?"
CONFDIR="?"
VAR_SPOOL_PPR="?"
NECHO="?"

# Where will the .pid files be found?
RUNDIR="$VAR_SPOOL_PPR/run"

# What language should messages be printed in?
lang=`$HOMEDIR/lib/ppr_conf_query internationalization daemonlang`
if [ -n "$lang" ]
    then
    LANG=$lang
    export LANG
    fi

case "$1" in

    start_msg)
	echo "Start PPR spooler"
	;;

    stop_msg)
	echo "Stopping PPR spooler"
	;;

    start)
	$NECHO -n "Starting PPR spooler: "

	if [ -n "$LANG" ]
	    then
	    $NECHO -n "(language is $LANG) "
	    fi

	# This is the spooler daemon.
	$HOMEDIR/bin/pprd && $NECHO -n "pprd "

	if [ -x $HOMEDIR/bin/papsrv -a -r $CONFDIR/papsrv.conf ]
	    then
	    $HOMEDIR/bin/papsrv && $NECHO -n "papsrv "
	    fi

	# Uncomment this if you want to run the new lprsrv
	# in standalone mode:
	#$HOMEDIR/lib/lprsrv -s printer && $NECHO -n "lprsrv "

	# Uncomment this if you want to run olprsrv in standalone mode:
	#$HOMEDIR/lib/olprsrv -s printer && $NECHO -n "olprsrv "

	# This updates links for lp, lpr, lprm, lpq, etc.
	$HOMEDIR/bin/uprint-newconf >/dev/null && $NECHO -n "uprint-newconf"

	echo

	# Extra code for RedHat Linux:
	if [ -d /var/lock/subsys ]; then touch /var/lock/subsys/ppr; fi
	;;

    stop)
	$NECHO -n "Stopping PPR daemons: "
	if [ -r $RUNDIR/pprd.pid ]
	    then
	    kill `cat $RUNDIR/pprd.pid` && $NECHO -n "pprd "
	    rm -f $RUNDIR/pprd.pid
	    fi
	if [ -r $RUNDIR/papsrv.pid ]
	    then
	    kill `cat $RUNDIR/papsrv.pid` && $NECHO -n "papsrv "
	    rm -f $RUNDIR/papsrv.pid
	    fi
	if [ -r $RUNDIR/lprsrv.pid ]
	    then
	    kill `cat $RUNDIR/lprsrv.pid` && $NECHO -n "lprsrv "
	    rm -f $RUNDIR/lprsrv.pid
	    fi
	if [ -r $RUNDIR/olprsrv.pid ]
	    then
	    kill `cat $RUNDIR/olprsrv.pid` && $NECHO -n "olprsrv "
	    rm -f $RUNDIR/olprsrv.pid
	    fi
	echo
	# For RedHat Linux:
	if [ -d /var/lock/subsys ]; then rm -f /var/lock/subsys/ppr; fi
	;;

    *)
	echo "Usage: ppr {start, stop}"
	exit 1
	;;
esac

exit 0

