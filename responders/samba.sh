#! @SHELL@
#
# mouse:~ppr/src/responders/samba.sh
# Copyright 1995--2005, Trinity College Computing Center.
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
# Last modified 28 March 2005.
#

#
# This responder sends to the user by means of Samba's smbclient.
# This responder accepts the address in two forms.	The first is just
# the NETBIOS name, the second is the NETBIOS name, a hyphen, and the IP
# address or the DNS name.
#

# Look in ppr.conf to find out where smbclient is.
SMBCLIENT=`lib/ppr_conf_query samba smbclient 0 /usr/local/samba/bin/smbclient`

#==============================
# Parse the command line
#==============================
while [ $# -gt 0 ]
	do
	name=`echo $1 | cut -d= -f1`
	value=`echo $1 | cut -d= -f2`
	case $name in
		for )
			for="$value"
			;;
		responder_address )
			responder_address="$value"
			;;
		responder_options )
			responder_options="$value"
			;;
		subject )
			long_message="$value"
			;;
		long_message )
			long_message="$value"
			;;
		esac
	shift
	done

#==============================
# Parse the responder options
#==============================
option_os=""
for opt in $responder_options
	do
	case $opt in
		os=* )
			option_os=`echo $opt | cut -d= -f2`
			;;
		* )
			;;
		esac
	done

#==============================
# Dispatch the message.
#==============================

# Break up the address:
nbname=`echo $responder_address | cut -d'-' -f1`
ip=`echo $responder_address | cut -d'-' -f2`

# Invoke smbclient to do the real work:
if [ -n "$ip" ]
	then
	echo "$long_message" | $SMBCLIENT -U ppr -M $nbname -I $ip >/dev/null
	else
	echo "$long_message" | $SMBCLIENT -U ppr -M $nbname >/dev/null
	fi

exit 0
