#! /bin/sh
#
# mouse:~ppr/src/filters_misc/troff_groff.sh
# Copyright 1995, 1996, 1997, 1998, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is" without
# express or implied warranty.
#
# Last Modified 22 December 1998.
#

#
# Troff filter for the PPR spooling system.
# This version uses Groff
# This filter ignores the options.
#
# $HOMEDIR/install/setup_filters passes this filter
# thru a sed script before installing it.
#

# Path of Groff.
GROFF="?"
GROG="?"

# This old code ran Groff on the standard input
# with an arbitrarily chosen set of options.
#$GROFF -man -Tps -etpR

# This new code, uses Groff's Grog to find the
# correct options.
COMMAND=`$GROG -Tps -msafer -`
lib/rewind_stdin
$COMMAND

# Pass on its exit value.
exit $?

# end of file

