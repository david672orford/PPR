#
# mouse:~ppr/src/fixup/fixup_links.sh
# Copyright 1995--2001, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appears in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is"
# without express or implied warranty.
#
# Last modified 26 November 2001.
#

BINDIR="?"
HOMEDIR="?"

#======================================================================
# Create links so that the PPR bin directory need
# not be in the PATH.
#======================================================================

echo "Creating symbolic links in \"$BINDIR\"..."
for i in ppr ppop ppad ppuser ppdoc ppr-xgrant
    do
    echo "    ln -s $HOMEDIR/bin/$i $BINDIR/$i"
    rm -f $BINDIR/$i
    ln -s $HOMEDIR/bin/$i $BINDIR/$i || exit 1
    done
echo "Done."
echo

exit 0

