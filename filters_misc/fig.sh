#! /bin/sh
#
# mouse.trincoll.edu:~ppr/src/misc_filters/fig.sh
# Copyright 1997, 1998, Trinity College Computing Center
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

# Paths of the filter.  This is filled
# in when the filter is installed.
FIG2DEV="?"

# Run it:
$FIG2DEV -L ps -P | sed -e '/^%%Title:/ d'

# Pass on exit value:
exit $?

# end of file
