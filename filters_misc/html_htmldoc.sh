#! /bin/sh
#
# mouse:~ppr/src/filters_misc/html_htmldoc.sh
# Copyright 1995--1999, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is"
# without express or implied warranty.
#
# Last modified 8 April 1999.
#

# This is changed to the path of HTMLDOC by setup_filters:
HTMLDOC="?"

# Run the filter:
$HTMLDOC --webpage --format ps1 -

# Pass on the exit code:
exit $?

