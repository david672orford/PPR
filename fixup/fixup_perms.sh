#! /bin/sh
#
# mouse:~ppr/src/fixup/fixup_perms.sh
# Copyright 1995--2003, Trinity College Computing Center.
# Written by David Chappell.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
# 
# * Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE 
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
# POSSIBILITY OF SUCH DAMAGE.
#
# Last modified 21 February 2003.
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
# First do a blanket change.
#=============================================================================

echo "Changing the owner of all PPR files to \"$USER_PPR\"..."

set_user_group_mode ()
    {
    echo "    Everything in $1"
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

echo

#=============================================================================
# Now go back and change things.
#=============================================================================

echo "Setting different permissions for certain files..."

do_chmod ()
    {
    echo "    chmod $1 $2"
    chmod $1 $2
    }

# This is done so that the DVI filters work.
do_chmod 775 $VAR_SPOOL_PPR/dvips

# These changes are made so that ppr-httpd can write its log
# as the user "pprwww", group "ppr".
do_chmod 775 $VAR_SPOOL_PPR/logs
do_chmod 664 $VAR_SPOOL_PPR/logs/ppr-httpd 2>/dev/null

# This is so that ordinary users can't see the PPR Popup registration
# information but the CGI script can write it.
do_chmod 770 "$VAR_SPOOL_PPR/pprpopup.db"

echo

#=============================================================================
# Make sure the programs which are supposed to be setuid or setgid to ppr
# or setuid to root are.
#=============================================================================

# These programs are all setuid ppr and setgid ppr.  We do the group
# too so that files they created will be owned by the right group.
echo "Setting up setuid/setgid $USER_PPR/$GROUP_PPR programs..."
for prog in \
	$HOMEDIR/bin/pprd \
	$HOMEDIR/bin/ppr \
	$HOMEDIR/bin/ppop \
	$HOMEDIR/bin/ppad \
	$HOMEDIR/bin/ppuser \
	$HOMEDIR/bin/ppr2samba \
	$HOMEDIR/bin/ppr-xgrant \
	$HOMEDIR/bin/papsrv \
	$HOMEDIR/bin/papd \
	$HOMEDIR/bin/ppr-passwd \
	$HOMEDIR/bin/uprint-lpr \
	$HOMEDIR/bin/uprint-lp
    do
    if [ -x $prog ]
	then
	echo "    $prog"
	chown $USER_PPR $prog
	chgrp $GROUP_PPR $prog
	chmod 6711 $prog
	fi
    done
echo

#
# The LPR interface must be setuid root so that it can allocate 
# priveledged sockets.  What a shame!
#
# sambaprint must be setuid root so that it can write to the
# Samba drive database.
#
echo "Setting up setuid root programs..."
for prog in \
	$HOMEDIR/interfaces/lpr \
	$HOMEDIR/lib/sambaprint
    do
    if [ -x $prog ]
	then
	echo "    $prog"
	chown root $prog
	chgrp $GROUP_PPR $prog
	chmod 4711 $prog
	fi
    done
echo

echo "Done."
echo

exit 0
