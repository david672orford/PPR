#
# mouse:~ppr/src/fixup/fixup_lmx.sh
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
# Last modified 2 July 2000.
#

#
# This script is called to execute code to integrate PPR with AT&T's
# LAN Manager for Unix, a producte which was used at Trinity College
# early in PPR's life.  It is used no longer and the code below
# may be broken.
#

HOMEDIR="?"
USER_PPR=?
NECHO=?

#===========================================================================
# Stuff for LAN Manager for Unix (LANMAN X)
#===========================================================================

# If we found /usr/bin/net, try to add a LAN Manager account.
if [ -x /usr/bin/net ]
    then

    echo "A LAN Manager account is now being added for PPR."
    $NECHO -n "Please supply a password for the new account PPR: "
    read pass
    net user $USER_PPR $pass /add /privilege:admin \
	/fullname:'PPR Spooling System' \
	/homedir:'c:\usr\ppr'

    if [ $? -ne 0 ]
	then
	echo "net user command failed!"
	exit 1
	fi

    echo
    echo "Logging PPR on to LAN Manager..."
    su $USER_PPR -c "net logon $USER_PPR $pass"
    echo "Done."
    echo

    echo "Creating a share name for client spooling..."
    net share 'pprclipr=c:\var\spool\ppr\pprclipr'
    net share pprclipr /remark:'PPR client spooling area'
    net access 'c:\var\spool\ppr\pprclipr' /add users:r guests:r
    echo "Done."
    echo

    #
    # If the LAN Manager X print processors directory
    # is present, install PPR's print processor.
    #
    echo "Installing the PPR LAN Manager X print processor..."
    if [ ! -d /var/opt/lanman/customs ]
        then
	echo "Directory /var/opt/lanman/customs/ppr doesn't exist!"
	exit 1
        fi
    cp $HOMEDIR/fixup/customs.ppr /var/opt/lanman/customs/ppr
    chown root /var/opt/lanman/customs/ppr
    chmod 755 /var/opt/lanman/customs/ppr
    echo "Done."
    echo

    fi

exit 0

