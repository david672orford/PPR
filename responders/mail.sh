#! @SHELL@
#
# mouse:~ppr/src/responders/mail.sh
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
# This responder sends a message to the user by means of electronic mail.
# When installed, this responder is linked to "errmail" as well.
#

# These are filled in when this script is installed:
EECHO="@EECHO@"
SENDMAIL_PATH="@SENDMAIL_PATH@"

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

# Pull in definitions of $RESP_*:
. lib/respond.sh

# Parse the responder options
option_printed=1
option_canceled=1
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
	* )
		;;
	esac
	done

# If invoked as "errmail" or with the printed=no option, bail out
# if we were about to report that a job has been printed:
if [ $option_printed -eq 0 -o `basename $0` = "errmail" ]
  then
  if [ $response_code_number -eq $RESP_FINISHED ]; then exit 0; fi
  fi

# If invoked with the canceled=no option, bail out if canceled.
if [ $option_canceled -eq 0 ]
  then
  if [ $response_code_number -eq $RESP_CANCELED_PRINTING ]; then exit 0; fi
  if [ $response_code_number -eq $RESP_CANCELED ]; then exit 0; fi
  fi

# Build a subject:
subject="${for}'s print job"

case $response_code_number in
	$RESP_FINISHED )
		subject="$subject printed";
		;;
	$RESP_ARRESTED )
		subject="$subject arrested";
		;;
	$RESP_CANCELED | $RESP_CANCELED_PRINTING )
		subject="$subject canceled";
		;;
	* )
		subject="$subject failed!";
		;;
esac

#====================================
# Expand on the canned message
#====================================

message=""

# If there is a reason parameter, add it.
if [ -n "$why_arrested" ]
	then
	message="$message$why_arrested\n"
	fi

# Possible add the title.
if [ -n "$title" ]
	then
	message="${message}\nThe title of this job is \"$title\".\n"
	fi

if [ "$pages_printed" != "?" ]		# if printed,
	then
	if [ $pages_printed -ne -1 ]	# if number of pages is know,
	then
	if [ $pages_printed -eq 1 ]
		then
		message="${message}\nIt is 1 page long."
		else
		message="${message}\nIt is $pages_printed pages long."
		fi

	if [ -n "$charge" ]
		then message="${message}  You have been charged $charge."; fi

	message="${message}\n"
	fi
	fi

# If it was submitted more than 10 minutes ago, tell when.
when=`lib/time_elapsed $time_submitted 600`
if [ -n "$when" ]
	then message="${message}\nYou submitted this job $when ago.\n"
	fi

job_log=`cat -`
if [ -n "$job_log" ]
  then
  message="$message\nYour job's log follows:\n------------------------------------------\n$job_log"
  fi

#====================================
# Dispatch the completed message
#====================================
$SENDMAIL_PATH $address <<END
From: PPR Spooler <$USER_PPR>
To: $for <$address>
Subject: $subject

$canned_message
`$EECHO "$message"`
END

# Done
exit 0

