#! /bin/sh
#
# mouse:~ppr/src/z_install_end/ppr.sh
# Copyright 1995--2004, Trinity College Computing Center.
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
# Last modified 13 May 2004.
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
HOMEDIR="?"
CONFDIR="?"
VAR_SPOOL_PPR="?"
EECHO="?"

# Read default file such as is found on Debian systems.
LPRSRV_STANDALONE_PORT=""
test -f /etc/default/ppr && . /etc/default/ppr

# Where will the .pid files be found?
RUNDIR="$VAR_SPOOL_PPR/run"

do_start ()
	{
	$EECHO "Starting PPR spooler: \c"

	# What language should messages be printed in?
	lang=`$HOMEDIR/lib/ppr_conf_query internationalization daemonlang`
	if [ -n "$lang" ]
		then
		$EECHO -n "(language is $lang) \c"
		LANG=$lang
		export LANG
		fi

	# This is the spooler daemon.
	$HOMEDIR/bin/pprd && $EECHO "pprd \c"

	# This is the new AppleTalk server daemon.
	if [ -x $HOMEDIR/bin/papd ]
		then
		$HOMEDIR/bin/papd && $EECHO "papd \c"
		fi

	# This is the old AppleTalk server daemon.
	if [ -x $HOMEDIR/bin/papsrv -a -r $CONFDIR/papsrv.conf ]
		then
		$HOMEDIR/bin/papsrv && $EECHO "papsrv \c"
		fi

	# Uncomment this if you want to run lprsrv in standalone mode.
	if [ -n "$LPRSRV_STANDALONE_PORT" ]
		then
		$HOMEDIR/lib/lprsrv -s $LPRSRV_STANDALONE_PORT && $EECHO "lprsrv \c"
		fi

	# This updates links for lp, lpr, lprm, lpq, etc.
	$HOMEDIR/bin/uprint-newconf >/dev/null && $EECHO "uprint-newconf \c"

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
	if [ -r $RUNDIR/papsrv.pid ]
		then
		kill `cat $RUNDIR/papsrv.pid` && $EECHO "papsrv \c"
		rm -f $RUNDIR/papsrv.pid
		fi
	if [ -r $RUNDIR/lprsrv.pid ]
		then
		kill `cat $RUNDIR/lprsrv.pid` && $EECHO "lprsrv \c"
		rm -f $RUNDIR/lprsrv.pid
		fi
	echo

	# For RedHat Linux:
	if [ -d /var/lock/subsys ]; then rm -f /var/lock/subsys/ppr; fi
	}

# This function prints the status of the deamon named in the first parameter.
# If it is down, this is noted only if the second parameter is true.
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

do_status()
	{
	do_status_1 pprd 1
	do_status_1 papd 0
	if [ -r $CONFDIR/papsrv.conf ]
		then
		do_status_1 papsrv 1
		else
		do_status_1 papsrv 0
		fi
	if [ -n "$LPRSRV_STANDALONE_PORT" ]
		then
		do_status_1 lprsrv 1
		else
		do_status_1 lprsrv 0
		fi
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

	restart|reload)
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
	exit 1
	;;
esac

exit 0

