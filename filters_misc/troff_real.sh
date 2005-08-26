#! /bin/sh
#
# mouse:~ppr/src/filters_misc/troff_real.sh
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
# Last modified 24 August 2005.
#

#
# Troff filter for the PPR spooling system.
#
# This version uses the Ditroff supplied with System V release 4
# and then passes the file through $FILTDIR/filter_ditroff.
#
# This program will have to be modified if your system uses a very
# old troff which emmits CAT/4 code.  In that case, you will have to
# change $FILTDIR/filter_ditroff to $FILTERS/filter_cat4 and create a
# working filter_cat4.
#

# The paths to the programs.
TBL="?"
REFER="?"
EQN="?"
PIC="?"
TROFF="?"
FILTDIR="?"

# Execute the whole lot
$TBL | $REFER | $EQN | $PIC | $TROFF -man | $FILTDIR/filter_ditroff

# Pass on the exit value of that wopping pipeline.
exit $?

# end of file

