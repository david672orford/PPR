#
# mouse:~ppr/src/libscript/interface.sh
# Copyright 1995--1999, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software is provided "as is" without express or
# implied warranty.
#
# Last modified 29 November 1999.
#

#
# This file is an excerpt from ~ppr/src/include/interface.h converted to
# borne shell script form.  This file is installed in the lib directory
# where the interface programs can include it.
#

EXIT_PRINTED=0
EXIT_PRNERR=1
EXIT_PRNERR_NORETRY=2
EXIT_JOBERR=3
EXIT_SIGNAL=4
EXIT_ENGAGED=5
EXIT_STARVED=6
EXIT_INCAPABLE=7

JOBBREAK_NONE=0
JOBBREAK_SIGNAL=1
JOBBREAK_CONTROL_D=2
JOBBREAK_PJL=3
JOBBREAK_SIGNAL_PJL=4
JOBBREAK_SAVE_RESTORE=5
JOBBREAK_NEWINTERFACE=6

CODES_UNKNOWN=0
CODES_Clean7Bit=1
CODES_Clean8Bit=2
CODES_Binary=3
CODES_TBCP=4

# end of file
