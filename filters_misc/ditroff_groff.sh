#! /bin/sh
#
# mouse:~ppr/src/filters_misc/ditroff_groff.sh
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
# Last modified 1 December 1998.
#

#
# This filter uses a component of GNU Troff to convert device
# independent Troff files to PostScript.
# Before being installed, this filter is passed thru a sed script
# by /usr/ppr/install/setup_filters.
#

# The path of grops.
GROPS=""

# Execute it on the standard input.
$GROPS

# Pass on its exit value.
exit $?

# end of file

