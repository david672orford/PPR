#! /bin/sh
#
# mouse.trincoll.edu:~ppr/src/misc_filters/ditroff_real.sh
# Copyright 1995, 1998, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software is provided "as is" without express or
# implied warranty.
#
# Last modified 4 September 1998.
#

#
# Convert Ditroff output to PostScript using the
# System V Ditroff filters.
#

DPOST="?"
POSTREVERSE="?"

$DPOST | $POSTREVERSE -r

exit $?

# end of file
