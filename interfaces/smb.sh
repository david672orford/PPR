#! /bin/sh
#
# mouse:~ppr/src/interfaces/smb.sh
# Copyright 1995--1999, Trinity College Computing Center.
# Written by Klaus Reimann.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is"
# without express or implied warranty.
#
# Last modified 1 July 1999.
#

#########################################################################
#
# This interface program uses Samba's smbclient to print to SMB
# (Microsoft LAN Manager, Windows) print queues.
#
# OPTIONS:
#   smbuser=
#   smbpassword=
#
# EXAMPLE
#   smbprint pprprinter '\\server\lp1' "smbuser=pprxx smbpassword=xx"
#
#########################################################################

# give the parameters names
PRINTER="$1"
ADDRESS="$2"
if [ "$PPR_GS_INTERFACE_HACK_ADDRESS" != "" ]; then ADDRESS="$PPR_GS_INTERFACE_HACK_ADDRESS"; fi
OPTIONS="$3"
if [ "$PPR_GS_INTERFACE_HACK_OPTIONS" != "" ]; then OPTIONS="$PPR_GS_INTERFACE_HACK_OPTIONS"; fi

echo "smb $PRINTER $ADDRESS $OPTINOS"

# Look in ppr.conf to find out where smbclient is.
SMBCLIENT=`lib/ppr_conf_query samba smbclient 0 /usr/local/samba/bin/smbclient`

# source the file which defines the exit codes
. lib/interface.sh
. lib/libppr_int.sh

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

