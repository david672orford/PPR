#! /bin/sh
#
# mouse:~ppr/src/fixup/fixup.sh
# Copyright 1995--2002, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appears in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is" without
# express or implied warranty.
#
# Last modified 4 January 2002.
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

# Install login profile scripts.
$HOMEDIR/fixup/fixup_login || exit 1

# Create a sample ppr.conf.
$HOMEDIR/fixup/fixup_conf || exit 1

# Create missing config file from samples.
$HOMEDIR/fixup/fixup_samples || exit 1

# Install init code.
$HOMEDIR/fixup/fixup_init || exit 1

# Install crontab.  This may fail.
su $USER_PPR -c $HOMEDIR/fixup/fixup_cron

# Set up Inetd.
$HOMEDIR/fixup/fixup_inetd || exit 1

# Create index of system fonts.
su $USER_PPR -c $HOMEDIR/bin/ppr-indexfonts || exit 1

# Create a base set of media descriptions.
$HOMEDIR/fixup/fixup_media || exit 1

# Create filter scripts using external programs.
$HOMEDIR/fixup/fixup_filters || exit 1

echo "PPR installation done.  You may now start the spooler."
echo

exit 0

