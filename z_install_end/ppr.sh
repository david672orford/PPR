#! /bin/sh
#
# mouse:~ppr/src/z_install_end/ppr.sh
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
# Last modified 5 March 2003.
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

do_start ()
	{
	$NECHO -n "Starting PPR spooler: "

	# What language should messages be printed in?
	lang=`$HOMEDIR/lib/ppr_conf_query internationalization daemonlang`
	if [ -n "$lang" ]
	    then
	    $NECHO -n "(language is $lang) "
	    LANG=$lang
	    export LANG
	    fi

	# This is the spooler daemon.
	$HOMEDIR/bin/pprd && $NECHO -n "pprd "

	# This is the new AppleTalk server daemon.
	if [ -x $HOMEDIR/bin/papd ]
	    then
	    $HOMEDIR/bin/papd
	    fi

	# This is the AppleTalk server daemon.
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
	}

do_stop ()
	{
	$NECHO -n "Stopping PPR daemons: "
	if [ -r $RUNDIR/pprd.pid ]
	    then
	    kill `cat $RUNDIR/pprd.pid` && $NECHO -n "pprd "
	    rm -f $RUNDIR/pprd.pid
	    fi
	if [ -r $RUNDIR/papd.pid ]
	    then
	    kill `cat $RUNDIR/papd.pid` && $NECHO -n "papd "
	    rm -f $RUNDIR/papd.pid
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
	}

case "$1" in

    start_msg)
	echo "Start PPR spooler"
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

    restart)
	do_stop
	do_start

    *)
	echo "Usage: ppr {start, stop, restart}"
	exit 1
	;;
esac

exit 0
