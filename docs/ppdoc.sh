#! /bin/sh
#
# mouse:~ppr/src/misc/ppdoc.sh
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
# Last modified 13 September 2000.
#

SHAREDIR="?"

if [ "$1" = "" ]
then
cat - <<EndOfHereFile
Usage: ppdoc <document-name>

The ppdoc command is a wrapper for your system's normal man(1) command.  It
sets MANPATH to the directory where the PPR man pages are located and then
runs man.

These are the PPR documents available in man format:
EndOfHereFile

for i in $SHAREDIR/man/man1/*.1 $SHAREDIR/man/man5/*.5 $SHAREDIR/man/man8/*.8
    do
    echo "   " `basename $i | sed -e 's/\..*$//'`
    done

exit 0
fi

MANPATH=$SHAREDIR/man
export MANPATH
man "$@"
exit $?

