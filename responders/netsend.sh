#! /bin/sh
#
# mouse:~ppr/src/responders/netsend.sh
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
# This responder attempts to reach the user by means of
# AT&T StarLAN LANManager net send.
#
# As you can see, it is behind the times.
#

# Give the arguments names:
for="$1"
address="$2"
canned_message="$3"

# Invoke that neat OS/2 program:
/usr/bin/net send $address "$canned_message"

# Done
exit 0
