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
# This commentator is designed to send the message using the write(1) command.
# If that fails, it calls the mail commentator.
#

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
   echo "write commentator: not important enough to send"
   echo "    $printer $code $cooked $raw1 $raw2"
   exit 0
   fi

# Try to use the write(1) command.
write $address <<EndOfMessage
To: $address
Subject: $printer $cooked

$printer: $code $cooked $raw1 $raw2

$canned_message
EndOfMessage

# If that didn't work, try the mail commentator.
if [ $? -ne 0 ]
    then
    exec commentator/mail "$@"
    fi

exit 0
