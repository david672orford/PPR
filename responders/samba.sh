#! /bin/sh
#
# mouse:~ppr/src/responders/samba.sh
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
# Last modified 3 April 2001.
#

#
# This responder sends to the user by means of Samba's smbclient.
# This responder accepts the address in two forms.  The first is just
# the NETBIOS name, the second is the NETBIOS name, a hyphen, and the IP
# address or the DNS name.
#

# Filled in by installscript:
EECHO="?"

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
    message="${message}Probable cause:  $why_arrested
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
    $EECHO  "$message" \
	| $SMBCLIENT -U ppr -M $NBNAME >/dev/null
    fi

exit 0
