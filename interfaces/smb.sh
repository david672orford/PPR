#! @SHELL@
#
# mouse:~ppr/src/interfaces/smb.sh
# Copyright 1995--2005, Trinity College Computing Center.
# Written by Klaus Reimann.
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
# Last modified 13 January 2005.
#

#########################################################################
#
# This interface program uses Samba's smbclient to print to SMB
# (Microsoft LAN Manager, Microsoft Windows) print queues.
#
# OPTIONS:
#	smbuser=
#	smbpassword=
#
# EXAMPLE
#	smbprint pprprinter '\\server\lp1' "smbuser=pprxx smbpassword=xx"
#
#########################################################################

# source the file which defines the exit codes
. lib/interface.sh
. lib/libppr_int.sh

if [ "$1" == "--probe" ]
	then
	echo "The interface program \"`basename $0`\" does not support probing." >&2
	exit $EXIT_PRNERR_NORETRY_BAD_SETTINGS
	fi

# give the parameters names
PRINTER="$1"
ADDRESS="$2"
if [ "$PPR_GS_INTERFACE_HACK_ADDRESS" != "" ]; then ADDRESS="$PPR_GS_INTERFACE_HACK_ADDRESS"; fi
OPTIONS="$3"
if [ "$PPR_GS_INTERFACE_HACK_OPTIONS" != "" ]; then OPTIONS="$PPR_GS_INTERFACE_HACK_OPTIONS"; fi

#echo "smb $PRINTER $ADDRESS $OPTIONS"

# Look in ppr.conf to find out where smbclient is.
SMBCLIENT=`lib/ppr_conf_query samba smbclient 0 /usr/local/samba/bin/smbclient`

# make sure the address parameter is not empty
case "${ADDRESS}x" in
  x)
	lib/alert "$PRINTER" TRUE "Address is empty"
	int_exit $EXIT_PRNERR_NORETRY
	;;
  '\\'?*'\'?*x)
	;;
  *)
	lib/alert "$PRINTER" TRUE 'Address must be of type \\server\printer'
	int_exit $EXIT_PRNERR_NORETRY
	;;
esac

# parse options
SMBUSER="ppr"
SMBPASSWORD=""
for opt in $OPTIONS
  do
  case "$opt" in
	smbuser=* )
	SMBUSER="`echo "$opt" | cut -d'=' -s -f2`"
	;;
	smbpassword=* )
	SMBPASSWORD="`echo "$opt" | cut -d'=' -s -f2`"
	;;
	* )
	lib/alert "$PRINTER" TRUE "Unrecognized interface option: $opt"
	int_exit $EXIT_PRNERR_NORETRY
	;;
  esac
done

#$SMBCLIENT "$ADDRESS" "$SMBPASSWORD" -c "print -" -P -N -U "$SMBUSER" -z 600000
$SMBCLIENT "$ADDRESS" "$SMBPASSWORD" -c "print -" -P -N -U "$SMBUSER"

retval=$?
case $retval in
  0 )
	int_exit $EXIT_PRINTED
	;;
  * )
	lib/alert "$PRINTER" TRUE "Smbclient failed, exit code: $retval"
	int_exit $EXIT_PRNERR
	;;
esac

# end of file

