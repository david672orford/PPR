#! /bin/sh
#
# mouse.trincoll.edu:~ppr/src/lanmanx/customs.ppr.sh
# Copyright 1995, 1998, Trinity College Computing Center.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software is provided "as is" without express or
# implied warranty.
#
# Last modified 16 September 1998.
#

#
# This script is a "print processor script" for use with AT&T LAN Manager/X
# which is a port of the OS/2 LAN Manager server to Unix.  I have not used
# LAN Manager/X in a long time.  For all I know it may be defunct.
#
# This scripts submits the job to ppr.  The responder is "NET SEND $CLIENT".
# By default, the name of the submitter is the client name but it can
# be overridden by a %%For: line.  Resources are stript
# out and placed in the cache, multiple copy requests are honoured,
# errors are reported by means of the responder.
# Destination switchsets are included by the -I switch.
#

# This will be changed when the script is installed:
HOMEDIR="?"

$HOMEDIR/bin/ppr -d $DEST -f -$CLIENT \
	-m netsend -r $CLIENT \
	-S true -R copies -e responder -I <$FILENAME &

exit 0

