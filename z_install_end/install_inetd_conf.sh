#! /bin/sh
#
# mouse:~ppr/src/z_install_end/install_inetd_conf.sh
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
# Last modified 5 February 2004.
#

#
# This script identifies the installed Inetd (Berkely, Linux, or Xinetd) and
# configures it to start PPR's lpr and http servers (though it leaves the lpd
# service disabled).  This script needs some more work to support RPM.
#

. ../makeprogs/paths.sh

GETSERVBYNAME="./getservbyname"
SERVICES="/etc/services"

INETD="/usr/sbin/inetd:/usr/etc/inetd"
INETD_CONF="/etc/inetd.conf"

XINETD="/usr/sbin/xinetd"
XINETD_CONF="/etc/xinetd.conf"
XINETD_D="/etc/xinetd.d"
XINETD_PPR="$XINETD_D/ppr"

#==========================================================================
# Sanity check function.
#==========================================================================
file_ok ()
	{
	./puts "  Checking for \"$1\"..."

	if [ -f $1 ]
		then
		echo " found"
		else
		if [ $2 == "-x" ]
			then
			echo " does not exist or is not executable, aborting."
			else
			echo " does not exist, aborting."
			fi
		exit 10
		fi

	if [ ! $2 $1 ]
		then
		case $2 in
			-w )
				echo "    (But not writable by you, lets hope we don't need to.)"
				;;
			-x )
				echo "Warning: The file \"$1\" is not executable."
				;;
			* )
				echo "Warning: The file \"$1\" is not $2."
				;;
		esac
		#exit 10
		fi
	}

prog_ok ()
	{
	for i in `echo $1 | tr ':' ' '`
		do
		./puts "  Checking for \"$i\"..."
		if [ -x $i ]
			then
			echo " found."
			return
			else
			echo " not found."
			fi
		done
	echo "No Inetd, aborting."
	exit 10
	}

#==========================================================================
# Add these lines to /etc/services if necessary.
#		pprpopup		15009/tcp
#		ppradmin		15010/tcp
#		pprcom			15011/tcp
#==========================================================================
add_service ()
	{
	./puts "  Checking for service \"$1\"... "
	port=`$GETSERVBYNAME $1 tcp`
	if [ $port -gt -1 ]
		then
		echo " already assigned port $port, good."
		else
		echo " not assigned, assigning port $2..."
		if [ ! -w $SERVICES ]
			then
			echo "Don't have write permission to $SERVICES!"
			exit 1
			fi
		echo "# Added by PPR" >>$SERVICES
		echo "$1 $2/tcp" >>$SERVICES
		if [ `$GETSERVBYNAME $1 tcp` -eq -1 ]
			then
			echo "Failed to add service \"$1\" (at port $2) to $SERVICES."
			echo "Perhaps this system doesn't use $SERVICES but uses"
			echo "NIS or some other database instead."
			exit 1
			fi
		fi
	}

#==========================================================================
# Add lines to /etc/inetd.conf as necessary.
#		printer stream tcp nowait root /usr/sbin/tcpd /usr/ppr/lib/lprsrv
#		ppradmin stream tcp nowait.400 pprwww /usr/sbin/tcpd /usr/ppr/lib/ppr-httpd
# Arguments are:
#		$1		portname
#		$2		.connexion_limit
#		$3		username
#		$4		program
#		$5		comment
#==========================================================================
add_inetd ()
	{
	if grep "^[# 	]*$1[ 	]" $INETD_CONF >/dev/null
		then
		echo "    Service \"$1\" is already in $INETD_CONF, good."
		else
		if [ ! -w $INETD_CONF ]
			then
			echo "  We have to write to \"$INETD_CONF\", please rerun as root."
			exit 1
			fi

		echo "    Adding service \"$1\" to $INETD_CONF."
		if [ "$inetd_type" = "extended" ]
			then
			i=$2
			else
			i=""
			fi
		echo "# $5" >>$INETD_CONF
		if [ -n "$tcpd" ]
			then
			echo "#$1 stream tcp nowait$i $3 $tcpd $4" >>$INETD_CONF
			else
			echo "#$1 stream tcp nowait$i $3 $4 `basename $4`" >>$INETD_CONF
			fi
		fi
	}

#==========================================================================
# Generate an Xinetd config file.
#==========================================================================
xinetd_config ()
	{
	cat - >$1 <<END
#
# Xinetd configuration file for PPR
#

# RFC 1179 (LPR/LPD) server
service printer
{
		socket_type		= stream
		wait			= no
		user			= root
		server			= $HOMEDIR/lib/lprsrv
		cps				= 400 30
		instances		= 50
		disable = yes
}

# WWW interface HTTP server
service ppradmin
{
		socket_type		= stream
		port			= 15010
		wait			= no
		user			= $USER_PPRWWW
		server			= $HOMEDIR/lib/ppr-httpd
		instances		= 50
		disable = no
}

# end of file
END
	}

#==========================================================================
# Send SIGHUP to the indicated program.
#==========================================================================

send_hup ()
  {
  if [ -n "$RPM_BUILD_ROOT" ]
    then
	echo "  Not sending SIGHUP to $prog because \$RPM_BUILD_ROOT is set."

	else
	prog=$1

	echo "  Sending SIGHUP to $prog..."

	# If program is nice,
	if [ -f /var/run/$prog.pid ]
		then
		echo "    Using PID file..."
		kill -1 `cat /var/run/$prog.pid`

	# If we have Linux utilities,
	elif [ -x /usr/bin/killall ]
		then
		echo "    Using killall..."
		/usr/bin/killall -HUP $prog

	# Do it the hard way.  Lets hope we have System V-style ps.
	else
		echo "    Using System V ps and awk to find pid of $prog..."
		pid=`ps -e | awk '/ '$prog'$/ { print $1 }'`
		if [ "$pid" != "" ]
			then
			echo "    PID is $pid"
			kill -1 $pid
		else
			echo "    not running"
		fi
	fi
  fi
  }
	
#==========================================================================
# /etc/services
#==========================================================================

# Make sure we have what we need to add services to /etc/services.
file_ok $GETSERVBYNAME -x
file_ok $SERVICES -w

# Add the standard printer service if it isn't present already.
add_service printer 515

# Add the PPR web managment port.
add_service ppradmin 15010

#==========================================================================
# If we are using Xinetd, things are pretty easy.
#==========================================================================
./puts "  Checking for \"$XINETD\"..."
if [ -x "$XINETD" -a -f "$XINETD_CONF" ]
	then
	echo " found, assuming it is what you are using..."

	# If we aren't making an RPM and /etc/xinetd.d doesn't exist yet,
	# create it and set up /etc/xinetd.conf to include it.
	if [ -z "$RPM_BUILD_ROOT" -a ! -d "$XINETD_D" ]
		then
		echo "Adding $XINETD_D..."
		mkdir "$XINETD_D" || exit 1
		echo "Adding include line to $XINETD_CONF..."
		echo "includedir /etc/xinetd.d" >>"$XINETD_CONF"
		fi

	# If the file /etc/xinetd.d/ppr doesn't exist, create it.  We pay
	# attention to $RPM_BUILD_ROOT.  If it is defined, we will have to
	# create /etc/xinetd.d before we can put a file in it.
	if [ -f $RPM_BUILD_ROOT$XINETD_PPR ]
		then
		echo "  $XINETD_PPR already exists, good."
		else
		echo "  Creating $XINETD_PPR..."
		if [ -n "$RPM_BUILD_ROOT" ]
			then
			mkdir -p $RPM_BUILD_ROOT/etc/xinetd.d
			fi
		xinetd_config $RPM_BUILD_ROOT$XINETD_PPR
		fi

	../makeprogs/installconf.sh root root 644 'config(noreplace)' $XINETD_PPR

	send_hup xinetd

	exit 0

	else
	echo " not found"
	fi

#==========================================================================
# See if we have what we need to run with plain old Inetd.
#==========================================================================
prog_ok $INETD
file_ok $INETD_CONF -w

# Find TCP wrappers.
./puts "  Looking for tcpd..."
tcpd=""
for d in /usr/local/sbin /usr/sbin
	do
	if [ -x "$d/tcpd" ]
		then
		tcpd="$d/tcpd"
		echo " found $tcpd."
		break
		fi
	done
if [ -z "$tcpd" ]
	then
	echo " not found, will do without."
	fi

# Figure out if Inetd is a nice Linux one or a nasty System V one.
./puts "  Checking Inetd version..."
if man inetd 2>&1 | grep 'wait\[\.max\]' >/dev/null
	then
	echo " modern"
	inetd_type="extended"
	else
	echo " antique"
	inetd_type="basic"
	fi

add_inetd printer ".400" root $HOMEDIR/lib/lprsrv "Uncomment this (after disabling lpd) to enable the PPR lpd server."
add_inetd ppradmin ".400" $USER_PPRWWW $HOMEDIR/lib/ppr-httpd "PPR's web managment server"

send_hup inetd

exit 0
