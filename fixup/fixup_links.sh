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
# Last modified 2 August 2001.
#

BINDIR="?"
HOMEDIR="?"
PROFILE_D="/etc/profile.d"

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

# Formerly we kept these files in $HOMEDIR/lib and put symbolic links in 
# /etc/profile.d, but it doesn't seem a good idea to have ppr writable login
# files in a place where root will execute them.
if [ -d $PROFILE_D ]
    then
    echo "Copying login files to \"$PROFILE_D\"..."
    for i in login_ppr.sh login_ppr.csh
        do
        echo "    cp $HOMEDIR/fixup/$i $PROFILE_D/$i"
        rm -f $PROFILE_D/$i
        cp $HOMEDIR/fixup/$i $PROFILE_D/$i || exit 1
	chown root $PROFILE_D/$i
	chmod 444 $PROFILE_D/$i
        done
    fi

echo "Done."
echo

exit 0

