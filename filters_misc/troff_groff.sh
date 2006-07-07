#! /bin/sh
#
# mouse:~ppr/src/filters_misc/troff_groff.sh
# Copyright 1995--2006, Trinity College Computing Center.
# Written by David Chappell.
#
# This file is part of PPR.  You can redistribute it and modify it under the
# terms of the revised BSD licence (without the advertising clause) as
# described in the accompanying file LICENSE.txt.
#
# Last modified 14 June 2006.
#

#
# Troff filter for the PPR spooling system.
#
# This version uses Groff.  This filter ignores the options.
#
# "ppr-index filters" passes this filter thru a sed script before installing it.
#

# Standard directories.
LIBDIR="?"

# Path of Groff.
GROFF="?"
GROG="?"

# This old code ran Groff on the standard input
# with an arbitrarily chosen set of options.
#$GROFF -man -Tps -etpR

# This new code, uses Groff's Grog to find the
# correct options.
COMMAND=`$GROG -Tps -msafer -`
$LIBDIR/rewind_stdin
$COMMAND

# Pass on its exit value.
exit $?

# end of file

