#! /bin/sh
#
# mouse:~ppr/src/commentators/mail.sh
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
# Last modified 4 May 2001.
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
