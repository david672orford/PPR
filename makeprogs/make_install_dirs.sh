#! /bin/sh
#
# mouse:~ppr/src/makeprogs/make_install_dirs.sh
# Copyright 1995--2002, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software is provided "as is" without express or
# implied warranty.
#
# Last modified 23 April 2002.
#

#
# This script is called by the top level PPR Makefile to
# create the directories necessary to build and install PPR.
#
# Changes to this should be coordinated with ../fixup/fixup_perms.sh.
#

# System configuration values:
. ../makeprogs/paths.sh

# List of files for RPM:
fileslist="`dirname $0`/installed_files_list"

# If global.mk is global.mk.unconfigured it won't
# contain a definition of HOMEDIR.
if [ -z "$HOMEDIR" ]
    then
    echo "You haven't run Configure yet."
    exit 1
    fi

# If we are building a binary RPM, make sure the root of the destination
# directory tree exists.
if [ -n "$RPM_BUILD_ROOT" -a ! -d "$RPM_BUILD_ROOT" ]
  then
  echo "The RPM_BUILD_ROOT \"$RPM_BUILD_ROOT\" does not exist!"
  exit 1
  fi

# This function returns "ok" if the directory in question
# exists and we have full access to it.
dirok ()
    {
    dir="$RPM_BUILD_ROOT$1"
    if [ -w $dir -a -r $dir -a -x $dir -a -d $dir ]
	then
	echo "ok"
	fi
    }

# This function creates a directory
directory ()
    {
    dir="$1"
    perms="$2"
    echo "$RPM_BUILD_ROOT$dir"
    if [ ! -d "$RPM_BUILD_ROOT$dir" ]
	then
	mkdir "$RPM_BUILD_ROOT$dir" || exit 1
	fi
    chown $USER_PPR "$RPM_BUILD_ROOT$dir" 2>/dev/null
    chgrp $GROUP_PPR "$RPM_BUILD_ROOT$dir" 2>/dev/null
    chmod $perms "$RPM_BUILD_ROOT$dir"
    echo "%dir \"$dir\"">>$fileslist
    }

# Try to make them but don't react badly if it doesn't work,
# that is handled in the next section.
mkdir -p $RPM_BUILD_ROOT$HOMEDIR		2>/dev/null
mkdir -p $RPM_BUILD_ROOT$SHAREDIR		2>/dev/null
mkdir -p $RPM_BUILD_ROOT$CONFDIR		2>/dev/null
mkdir -p $RPM_BUILD_ROOT$VAR_SPOOL_PPR		2>/dev/null
mkdir -p $RPM_BUILD_ROOT$TEMPDIR 		2>/dev/null

# Make sure the directories exist
if [ -z "`dirok $HOMEDIR`" -o -z "`dirok $SHAREDIR`" -o -z "`dirok $CONFDIR`" -o -z "`dirok $VAR_SPOOL_PPR`" -o -z "`dirok $TEMPDIR`" ]
    then
    echo "Before this script can be run, root must create the directories $HOMEDIR,"
    echo "$SHAREDIR, $VAR_SPOOL_PPR and $CONFDIR and make sure that they are"
    echo "writable by you."
    exit 1
    fi

echo "%dir \"$HOMEDIR\"">>$fileslist
echo "%dir \"$SHAREDIR\"">>$fileslist
echo "%dir \"$CONFDIR\"">>$fileslist
echo "%dir \"$VAR_SPOOL_PPR\"">>$fileslist

# It is necessary to create empty configuration
# directories.
directory $CONFDIR/printers 755
directory $CONFDIR/groups 755
directory $CONFDIR/aliases 755
directory $CONFDIR/mounted 755
directory $CONFDIR/acl 755

# Make the directories for the permanent cache.
directory $SHAREDIR/cache 755
directory $SHAREDIR/cache/font 755
directory $SHAREDIR/cache/procset 755
directory $SHAREDIR/cache/file 755
directory $SHAREDIR/cache/encoding 755

# Make the miscelaineous directories in /usr/lib/ppr.
directory $HOMEDIR/bin 755
directory $HOMEDIR/lib 755
directory $HOMEDIR/filters 755
directory $HOMEDIR/interfaces 755
directory $HOMEDIR/responders 755
directory $HOMEDIR/commentators 755
directory $HOMEDIR/fixup 755
directory $HOMEDIR/editps 755

# Architecture indendent stuff
directory $SHAREDIR/PPDFiles 755
directory $SHAREDIR/fonts 755
directory $SHAREDIR/man 755
directory $SHAREDIR/lib 755
directory $SHAREDIR/locale 755

# Make the directories in the spool area.
directory $VAR_SPOOL_PPR/queue 700
directory $VAR_SPOOL_PPR/jobs 700
directory $VAR_SPOOL_PPR/run 755
directory $VAR_SPOOL_PPR/printers 755
directory $VAR_SPOOL_PPR/printers/alerts 755
directory $VAR_SPOOL_PPR/printers/status 755
directory $VAR_SPOOL_PPR/printers/addr_cache 755
directory $VAR_SPOOL_PPR/logs 775	# <-- group can write
directory $VAR_SPOOL_PPR/cache 755
directory $VAR_SPOOL_PPR/cache/font 755
directory $VAR_SPOOL_PPR/cache/procset 755
# The next one is commented out because to many drivers are too ill-behaved
# and generate multiple files with the same name.
#directory $VAR_SPOOL_PPR/cache/file
directory $VAR_SPOOL_PPR/cache/encoding 755
directory $VAR_SPOOL_PPR/dvips 755
directory $VAR_SPOOL_PPR/drivers 755
directory $VAR_SPOOL_PPR/drivers/win95 755
directory $VAR_SPOOL_PPR/drivers/macos 755
directory $VAR_SPOOL_PPR/sambaspool 1777
directory $VAR_SPOOL_PPR/pprclipr 755
directory $VAR_SPOOL_PPR/pprpopup.db 770	# <-- group can write

# Make the directories for web documentation and managment tools
directory $HOMEDIR/cgi-bin 755
directory $SHAREDIR/www 755

exit 0

