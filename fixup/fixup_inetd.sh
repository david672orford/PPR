#! /bin/sh
#
# mouse:~ppr/src/fixup/fixup_inetd.sh
# Copyright 1995--2001, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appears in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is"
# without express or implied warranty.
#
# Last modified 11 May 2001.
#

HOMEDIR="?"
USER_PPR="?"
USER_PPRWWW="?"

GETSERVBYNAME="$HOMEDIR/lib/getservbyname"
SERVICES="/etc/services"
INETD="/usr/sbin/inetd"
INETD_CONF="/etc/inetd.conf"
XINETD_D="/etc/xinetd.d"

echo "Setting up Inetd for PPR services..."

# Sanity check function.
file_ok ()
    {
    echo "  Checking for \"$1\"..."

    if [ ! -f $1 ]
    	then
	if [ $2 == "-x" ]
	    then
	    echo "The program \"$1\" does not exist."
	    else
	    echo "The file \"$1\" does not exist."
	    fi
	exit 10
	fi

    if [ ! $2 $1 ]
	then
	case $2 in
	    -w )
	    	echo "The file \"$1\" is not writable."
	    	;;
	    -x )
	    	echo "The file \"$1\" is not executable."
	    	;;
	    * )
	    	echo "The file \"$1\" is not $2."
		;;
	esac
	#exit 10
    	fi
    }

# Make sure we have what we need to add the services to /etc/services.
file_ok $GETSERVBYNAME -x
file_ok $SERVICES -w

# Add these lines to /etc/services if necessary.
#	pprpopup	15009/tcp
#	ppradmin	15010/tcp
#	pprcom		15011/tcp
add_service ()
    {
    port=`$GETSERVBYNAME $1 tcp`
    if [ $port -gt -1 ]
	then
	echo "  Service \"$1\" is already known (at port $port), good."
	else
	echo "  Adding service \"$1\" (at port $2) to $SERVICES."
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
add_service printer 515
add_service ppradmin 15010

if [ -f /usr/sbin/xinetd -a -d /etc/xinetd.d ]
    then
    file="/etc/xinetd.d/ppr"
    if [ -f $file ]
	then
	echo "  $file already exists, good."
	else
	echo "  Creating $file..."
	cat - >$file <<END
#============================================================================
# Xinetd configuration file for PPR
#============================================================================

# RFC 1179 (LPR/LPD) server
service printer
{
	disable	= no
	socket_type	= stream
	wait		= no
	user		= root
	server		= $HOMEDIR/lib/lprsrv
	cps		= 400 30
	instances	= 50
	disabled	= yes
}

# WWW interface HTTP server
service ppradmin
{
	disable	= no
	socket_type	= stream
	wait		= no
	user		= $USER_PPRWWW
	server		= $HOMEDIR/lib/ppr-httpd
	instances	= 50
	disabled	= yes
}

# end of file
END
	fi
else

# See if we have what we need to run with plain old Inetd.
file_ok $INETD_CONF -w
file_ok $INETD -x

# Find TCP wrappers.
echo "  Looking for tcpd..."
tcpd=""
for d in /usr/local/sbin /usr/sbin
    do
    if [ -x "$d/tcpd" ]
    	then
	tcpd="$d/tcpd"
	echo "    Found $tcpd."
	break
    	fi
    done
if [ -z "$tcpd" ]
    then
    echo "    Not found."
    fi

# Figure out if Inetd is a nice Linux one or a nasty System V one.
echo "  Checking Inetd version..."
if man inetd | grep 'wait\[\.max\]' >/dev/null
    then
    inetd_type="extended"
    else
    inetd_type="basic"
    fi
echo "    Inetd is $inetd_type."

# Add lines to /etc/inetd.conf as necessary.
#	printer stream tcp nowait root /usr/sbin/tcpd /usr/ppr/lib/lprsrv
#	pprpopup stream tcp nowait ppr /usr/sbin/tcpd /usr/ppr/lib/pprpopupd
#	ppradmin stream tcp nowait.400 pprwww /usr/sbin/tcpd /usr/ppr/lib/ppr-httpd
#	pprcom stream tcp nowait nobody /usr/sbin/tcpd /usr/ppr/lib/ppr-commentary-httpd
add_inetd ()
    {
    if grep "^$1[ 	]" $INETD_CONF >/dev/null
    	then
    	echo "  Service \"$1\" is already in $INETD_CONF, good."
    	else
	echo "  Adding service \"$1\" to $INETD_CONF."
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
add_inetd printer ".400" root $HOMEDIR/lib/lprsrv "Uncomment this (after disabling lpd) to enable the PPR lpd server."
add_inetd ppradmin ".400" $USER_PPRWWW $HOMEDIR/lib/ppr-httpd "PPR's web managment server"

fi

echo "Done."
echo
exit 0
