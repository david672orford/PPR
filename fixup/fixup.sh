#! /bin/sh
#
# mouse:~ppr/src/fixup/fixup.sh
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

#
# This script performs the final stage of PPR installation.  It doesn't do
# any actual work, it just calls a bunch of scripts.  It is done this way
# for ease of mantainance and for the convienence of binary distribution
# builders who may want to execute just a part of this fixup procedure or
# to substitute distribution-specific code for part of it.  Hopefully it
# will also make it easier for people to contribute code to this install
# program.
#

HOMEDIR="?"
SHAREDIR="?"
CONFDIR="?"
VAR_SPOOL_PPR="?"
BINDIR="?"
NECHO="?"
USER_PPR=?

# We have to be root to create users, change ownership, and
# create setuid programs.
id | egrep '^uid=0\(' >/dev/null
if [ $? -ne 0 ]
    then
    echo "You must run this script as root."
    exit 1
    fi

# Do a little more sanity checking.
for dir in $HOMEDIR $SHAREDIR $CONFDIR $VAR_SPOOL_PPR
    do
    if [ ! -d $dir ]
        then
        echo "The directory \"$dir\" does not exist.  You should not attempt"
        echo "to run this script until all the PPR files are in place."
        exit 1
        fi
    done

# Create the accounts and groups need by PPR.
$HOMEDIR/fixup/fixup_accounts || exit 1

# Set the ownership and access permissions on PPR files.
$HOMEDIR/fixup/fixup_perms || exit 1

# Remove files from older versions of PPR.
$HOMEDIR/fixup/fixup_obsolete || exit 1

# Setup up for AT&T LAN Manager for Unix.
$HOMEDIR/fixup/fixup_lmx || exit 1

# Create links in public bin directory.
$HOMEDIR/fixup/fixup_links || exit 1

# Install init code.
$HOMEDIR/fixup/fixup_init || exit 1

# Install crontab.  This may fail.
su $USER_PPR -c $HOMEDIR/fixup/fixup_cron

# Set up Inetd.  It it fails, maybe there is no Inetd installed.
$HOMEDIR/fixup/fixup_inetd

echo "PPR installation done.  You may now start the spooler."
echo

exit 0

