#! /bin/sh
#
# mouse:~ppr/src/init_and_cron/cron_daily.sh
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
# Last modified 20 November 2000.
#


HOMEDIR="?"
VAR_SPOOL_PPR="?"

$HOMEDIR/bin/ppr-indexfonts >$VAR_SPOOL_PPR/logs/ppr-indexfonts 2>&1

$HOMEDIR/bin/ppr-clean >$VAR_SPOOL_PPR/logs/ppr-clean 2>&1

exit 0
