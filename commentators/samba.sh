#! /bin/sh
#
# mouse:~ppr/src/commentators/samba.sh
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
# Last modified 4 May 2001.
#

#
# This commentator uses Samba's smbclient.  No attempt is made to format the 
# message in a pretty or readable fashion.  The address must be in one of 
# these forms:
#
# COMPUTERNAME
# COMPUTERNAME-10.0.0.10
# COMPUTERNAME-computername.myorg.org
#
# Use the first form only if you are sure that smbclient is set up to
# resolve NetBIOS names correctly.
#

# Give names to the parameters:
address="$1"
options="$2"
printer="$3"
code="$4"
cooked="$5"
raw1="$6"
raw2="$7"

# Parse the options.
severity_threshold=3
for opt in $options
   do
   case $opt in
	severity=* )
	    severity_threshold=`echo $opt | cut -d'=' -f2`
	    ;;
   esac
   done

# If this message isn't severe enough, bail out.
if [ $severity -lt $severity_threshold ]
   then
   echo "samba commentator: not important enough to send:"
   echo "    $printer $code $cooked $raw1 $raw2"
   exit 0
   fi

# Look in ppr.conf to find out where smbclient is.
SMBCLIENT=`lib/ppr_conf_query samba smbclient 0 /usr/local/samba/bin/smbclient`

# Break up the address:
NBNAME=`echo $address | cut -d'-' -f1`
IP=`echo $address | cut -d'-' -f2`
if [ -n "$IP" ]; then IP="-I $IP"; fi

# Run smbclient:
/bin/echo "$printer: $cooked\n\nPlease press Control-D to remove this message" \
	| $SMBCLIENT -U ppr -M $NBNAME $IP >/dev/null

exit 0

