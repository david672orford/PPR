#
# mouse:~ppr/src/fixup/fixup_perms.sh
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
# Last modified 20 February 2001.
#

HOMEDIR="?"
CONFDIR="?"
SHAREDIR="?"
VAR_SPOOL_PPR="?"
USER_PPR=?
GROUP_PPR=?

#=============================================================================
# Make sure the required users exist.
#=============================================================================

echo "Checking for required users and groups..."

if [ `$HOMEDIR/lib/getpwnam $USER_PPR` -eq -1 ]
    then
    echo "The user \"$USER_PPR\" does not exist."
    exit 1
    fi

if [ `$HOMEDIR/lib/getgrnam $GROUP_PPR` -eq -1 ]
    then
    echo "The group \"$GROUP_PPR\" does not exist."
    exit 1
    fi

echo "Done."
echo

#=============================================================================
# Change a lot of file permisions
#=============================================================================

echo "Changing the owner of all PPR files to \"$USER_PPR\"..."

set_user_group_mode ()
    {
    chown $USER_PPR $1
    chgrp $GROUP_PPR $1
    chmod 755 $1
    chown -R $USER_PPR $1/*
    chgrp -R $GROUP_PPR $1/*
    }

set_user_group_mode $CONFDIR
set_user_group_mode $HOMEDIR
set_user_group_mode $SHAREDIR
set_user_group_mode $VAR_SPOOL_PPR

# This is done so that the DVI filters work.
chmod 775 $VAR_SPOOL_PPR/dvips

# These changes are made so that ppr-httpd can write its log
# as the user "pprwww", group "ppr".
chmod 775 $VAR_SPOOL_PPR/logs
chmod 664 $VAR_SPOOL_PPR/logs/ppr-httpd 2>/dev/null

# This is so that ordinary users can't see the PPR Popup registration
# information but the CGI script can write it.
chmod 770 "$VAR_SPOOL_PPR/pprpopup.db"

echo "Done."
echo

#=============================================================================
# Make sure the programs which are supposed
# to be setuid to ppr and setgid to ppop are.
#=============================================================================

echo "Setting up setuid programs..."

chmod 6711 $HOMEDIR/bin/pprd \
	$HOMEDIR/bin/ppr \
	$HOMEDIR/bin/ppop \
	$HOMEDIR/bin/ppad \
	$HOMEDIR/bin/ppuser \
	$HOMEDIR/bin/ppr2samba

if [ -x $HOMEDIR/bin/papsrv ]
    then
    chmod 6711 $HOMEDIR/bin/papsrv
    fi

# The LPR interface must be setuid root
# so that it can allocate privledged sockets.
chown root $HOMEDIR/interfaces/lpr
chmod 4711 $HOMEDIR/interfaces/lpr

# The ppr-xgrant program must be setuid root too.
chown root $HOMEDIR/bin/ppr-xgrant
chmod 4711 $HOMEDIR/bin/ppr-xgrant

# The uprint programs must be setuid root too.
chown root $HOMEDIR/bin/uprint-*
chmod 4711 $HOMEDIR/bin/uprint-*

echo "Done."
echo

exit 0

