#
# mouse:~ppr/src/fixup/fixup_login.sh
# Copyright 1995--2002, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appears in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is"
# without express or implied warranty.
#
# Last modified 4 January 2002.
#

HOMEDIR="?"
PROFILE_D="/etc/profile.d"

#======================================================================
# Create links so that the PPR bin directory need
# not be in the PATH.
#======================================================================

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
	chmod 555 $PROFILE_D/$i
        done
    echo "Done."
    echo
    fi

exit 0

