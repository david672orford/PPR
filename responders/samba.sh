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
# Last modified 13 January 2005.
#

#
# This responder sends to the user by means of Samba's smbclient.
# This responder accepts the address in two forms.	The first is just
# the NETBIOS name, the second is the NETBIOS name, a hyphen, and the IP
# address or the DNS name.
#

# Filled in by installscript:
EECHO="@EECHO@"

# Look in ppr.conf to find out where smbclient is.
SMBCLIENT=`lib/ppr_conf_query samba smbclient 0 /usr/local/samba/bin/smbclient`

# Pull in definitions of $RESP_*:
. lib/respond.sh

# Give names to all the parameters.
for="$1"
address="$2"
canned_message="$3"
canned_message2="$4"
responder_options="$5"
response_code_number="$6"
jobid="$7"
extra="$8"
title="$9"
shift 9
time_submitted="$1"
why_arrested="$2"
pages_printed="$3"
charge="$4"

# Parse the responder options
option_printed=1
option_canceled=1
option_os=""
for opt in $responder_options
	do
	case $opt in
	printed=[nNfF0]* )
		option_printed=0
		;;
	printed=[yYtT1-9]* )
		option_printed=1
		;;
	canceled=[nNfF0]* )
		option_canceled=0
		;;
	canceled=[yYtT1-9]* )
		option_canceled=1
		;;
	os=* )
		option_os=`echo $opt | cut -d= -f2`
		;;
	* )
		;;
	esac
	done

# If invoked with the printed=no option, bail out
# if we were about to report that a job has been printed:
if [ $option_printed -eq 0 ]
  then
  if [ $response_code_number -eq $RESP_FINISHED ]; then exit 0; fi
  fi

# If invoked with the canceled=no option, bail out if canceled.
if [ $option_canceled -eq 0 ]
  then
  if [ $response_code_number -eq $RESP_CANCELED_PRINTING ]; then exit 0; fi
  if [ $response_code_number -eq $RESP_CANCELED ]; then exit 0; fi
  fi

#=========================================
# Construct the message.
#=========================================

message="$canned_message
";

if [ -n "$why_arrested" ]
	then
	message="${message}Probable cause:	$why_arrested
"
	fi

# Leave a blank line.
message="$message
";

# Possibly add the title.
if [ -n "$title" ]
	then
	message="${message}The title of this job is \"$title\".
"
	fi

if [ "$pages_printed" != "?" ]
	then
	if [ $pages_printed -ne -1 ]	# if number of pages is known,
	then
	if [ $pages_printed -eq 1 ]
		then
		message="${message}It is 1 page long."
		else
		message="${message}It is $pages_printed pages long."
		fi

	if [ -n "$charge" ]
		then message="${message}  You have been charged $charge."; fi

	message="${message}\n"
	fi
	fi

# If it was submitted more than 10 minutes ago, tell when.
when=`lib/time_elapsed $time_submitted 600`
if [ -n "$when" ]
	then
	message="${message}You submitted this job $when ago.
"
	fi

# If winpopup is used for this os, add a message
# explaining the easiest way to remove the message.
if [ "$option_os" = "WfWg" -o "$option_os" = "Win95" ]
  then
  message="$message
Please press Control-D to remove this message."
  fi

#=============================================
# Dispatch the message.
#=============================================

# Break up the address:
NBNAME=`echo $address | cut -d'-' -f1`
IP=`echo $address | cut -d'-' -f2`

# Invoke smbclient to do the real work:
if [ -n "$IP" ]
	then
	$EECHO "$message" \
	| $SMBCLIENT -U ppr -M $NBNAME -I $IP >/dev/null
	else
	$EECHO	"$message" \
	| $SMBCLIENT -U ppr -M $NBNAME >/dev/null
	fi

exit 0
