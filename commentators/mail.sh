#! /bin/sh
#
# mouse:~ppr/src/commentators/mail.sh
# Copyright 1995--2003, Trinity College Computing Center.
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
# Last modified 5 April 2003.
#

#
# This commentator is designed to send the message via email.
#

# Filled in at install time:
SENDMAIL_PATH=?
USER_PPR=?

# Give the perameters names.
address="$1"
options="$2"
printer="$3"
code="$4"
cooked="$5"
raw1="$6"
raw2="$7"
severity="$8"
canned_message="$9"

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
   echo "mail commentator: not important enough to send:"
   echo "    $printer $code $cooked $raw1 $raw2"
   exit 0
   fi

$SENDMAIL_PATH $address <<EndOfMessage
From: PPR at `uname -n | cut -d. -f1` <$PPR_USER>
To: <$address>
Subject: $printer $cooked

$printer: $code $cooked $raw1 $raw2

$canned_message
EndOfMessage

exit 0
