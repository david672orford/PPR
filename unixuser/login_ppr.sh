#
# mouse:~ppr/src/unixuser/login_ppr.sh
# Copyright 1995--2000, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appears in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is"
# without express or implied warranty.
#
# Last modified 26 May 2000.
#

#
# This script should be included in /etc/profile so that these PPR
# environment variables are set at each login.
#

if [ "$DISPLAY" != "" ]
	then
	ppr-xgrant
	PPR_RESPONDER="xwin"
	PPR_RESPONDER_ADDRESS="$DISPLAY"
	PPR_RESPONDER_OPTIONS=""
	export PPR_RESPONDER PPR_RESPONDER_ADDRESS PPR_RESPONDER_OPTIONS
	fi

# end of file

