#! @SHELL@
#
# mouse:~ppr/src/commentators/samba.sh
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
# Last modified 13 January 2005.
#

#
# This commentator uses Samba's smbclient.	No attempt is made to format the 
# message in a pretty or readable fashion.	The address must be in one of 
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

