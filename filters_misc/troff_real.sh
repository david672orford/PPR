#! /bin/sh
#
# mouse.trincoll.edu:~ppr/src/misc_filters/troff_real.sh
# Copyright 1995, 1998, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is" without
# express or implied warranty.
#
# Last Modified 4 September 1998.
#

#
# Troff filter for the PPR spooling system.
#
# This version uses the Ditroff supplied with System V release 4
# and then passes the file through "filters/filter_ditroff".
#
# This program will have to be modified if your system uses a very
# old troff which emmits CAT/4 code.  In that case, you will have to
# change "filters/filter_ditroff" to "filters/filter_cat4" and create a
# working "filters/filter_cat4".
#

# The paths to the programs.
TBL="?"
REFER="?"
EQN="?"
PIC="?"
TROFF="?"

# Execute the whole lot
$TBL | $REFER | $EQN | $PIC | $TROFF -man | filters/filter_ditroff

# Pass on the exit value of that wopping pipeline.
exit $?

# end of file

