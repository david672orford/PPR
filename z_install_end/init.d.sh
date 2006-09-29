#! @SHELL@
#
# mouse:~ppr/src/z_install_end/ppr.sh
# Copyright 1995--2006, Trinity College Computing Center.
# Written by David Chappell.
#
# This file is part of PPR.  You can redistribute it and modify it under the
# terms of the revised BSD licence (without the advertising clause) as
# described in the accompanying file LICENSE.txt.
#
# Last modified 21 September 2006.
#

#
# This script is designed to work with the System V init provided with operating
# systems such as SunOS 5 and the reimplementation used in most Linux 
# distributions.  If invoked with the argument "start", it starts PPRD and 
# PAPSRV, if invoked with the argument "stop", it is supposed to stop them.
#

#
# This information in this following comments is read by the Linux program 
# /sbin/chkconfig which can be used to automatically install and remove the
# links from the rc?.d directories to the init.d directory.
#
# chkconfig: 2345 80 20
# description: PPR is a print spooler for PostScript printers.
# probe: true
#

# System configuration values in this section are filled in when the
# script is installed.
LIBDIR="@LIBDIR@"
CONFDIR="@CONFDIR@"
VAR_SPOOL_PPR="@VAR_SPOOL_PPR@"
BINDIR="@BINDIR@"
RUNDIR="@RUNDIR@"
EECHO="@EECHO@"
USER_PPR="@USER_PPR@"
GROUP_PPR="@GROUP_PPR@"

# Bail out if PPR isn't installed.  (On Debian systems, this initscript is
# considered to be a configuration file and isn't deleted if the package is
# simply removed, only if it is purged.)  Exit code 5 is required by LSB.
# Debian policy supposedly requires exit code 0.
test -x $BINDIR/pprd || exit 5

# If a Debian-style defaults file exists, read it it and let it override
# the values defined here.
LPRSRV_STANDALONE_LISTEN=""
IPP_STANDALONE_LISTEN=""
ADMIN_STANDALONE_LISTEN=""
test -f /etc/default/ppr && . /etc/default/ppr

do_start ()
	{
	$EECHO "Starting PPR spooler: \c"

	# What language should messages be printed in?
	lang=`$LIBDIR/ppr_conf_query internationalization daemonlang`
	if [ -n "$lang" ]
		then
		$EECHO -n "(language is $lang) \c"
		LANG=$lang
		export LANG
		fi

	# If /var/run is a tmpfs, we must create our run directory
	# at every system restart.
	if [ ! -d $RUNDIR ]
		then
		mkdir $RUNDIR
		chown $USER_PPR $RUNDIR
		chgrp $GROUP_PPR $RUNDIR
		chmod 775 $RUNDIR
		fi

	# This is the spooler daemon.
	$BINDIR/pprd && $EECHO "pprd \c"

	# This is the new AppleTalk server daemon.  We start it if it was built
	# and installed.
	if [ -x $BINDIR/papd ]
		then
		$BINDIR/papd && $EECHO "papd \c"
		fi

	echo

	# Extra code for RedHat Linux:
	if [ -d /var/lock/subsys ]; then touch /var/lock/subsys/ppr; fi
	}

do_stop ()
	{
	$EECHO  "Stopping PPR daemons: \c"
	if [ -r $RUNDIR/pprd.pid ]
		then
		kill `cat $RUNDIR/pprd.pid` && $EECHO "pprd \c"
		rm -f $RUNDIR/pprd.pid
		fi
	if [ -r $RUNDIR/papd.pid ]
		then
		kill `cat $RUNDIR/papd.pid` && $EECHO "papd \c"
		rm -f $RUNDIR/papd.pid
		fi
	echo

	# For RedHat Linux:
	if [ -d /var/lock/subsys ]; then rm -f /var/lock/subsys/ppr; fi
	}

# This function prints the status of the deamon named in the first parameter.
# If it is down, this is noted only if the second parameter is not zero.
# A daemon is considered to be up if its .pid file exists and kill -0 works
# on its PID.
do_status_1()
	{
	proc=$1
	important=$2
	if [ -f $RUNDIR/$proc.pid ] && kill -0 `cat $RUNDIR/$proc.pid` 2>/dev/null
		then
		echo "$proc up since" `ls -l $RUNDIR/$proc.pid | awk '{ print $6, $7 }'`
		else
		if [ $important -ne 0 ]
			then
			echo "$proc down"
			fi
		fi
	}

# Implementation of status subcommand
do_status()
	{
	do_status_1 pprd 1
	do_status_1 papd 0
	}

case "$1" in

	start_msg)
	echo "Startint PPR spooler"
	;;

	stop_msg)
		echo "Stopping PPR spooler"
		;;

	start)
		do_start
		;;

	stop)
		do_stop
		;;

	status)
		do_status
		;;

	# reload and force-reload should be made more gentle
	restart|reload|force-reload)
		do_stop
		do_start
		;;

	condrestart)
		;;

	probe)
		if [ -f $RUNDIR/pprd.pid ]
			then
			echo "restart"
			fi
		;;

	*)
		echo "Usage: ppr {start|stop|status|reload|restart|probe}"
		exit 2
		;;
esac

exit 0

