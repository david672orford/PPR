#! /bin/sh
#
# mouse:~ppr/src/z_install_begin/install_create_dirs.sh
# Copyright 1995--2005, Trinity College Computing Center.
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
# Last modified 11 March 2005.
#

#=============================================================================
# This script is called by the top level PPR Makefile to create the 
# directories necessary to build and install PPR.
#=============================================================================

# Blank the list of files for RPM.
fileslist="`dirname $0`/../z_install_begin/installed_files_list"
rm -f $fileslist

# If global.mk is global.mk.unconfigured it won't
# contain a definition of LIBDIR.
if [ -z "$LIBDIR" ]
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
	echo "%attr(-,$USER_PPR,$GROUP_PPR) %dir \"$dir\"">>$fileslist
	}

#
# Create the top-level PPR directories.  Note that we won't be able to change
# owner and group if we are building an RPM package as a user other than
# root or $USER_PPR.
#
for dir in $CONFDIR $LIBDIR $SHAREDIR $VAR_SPOOL_PPR
	do
	if [ ! -d $RPM_BUILD_ROOT$dir ]
		then
		mkdir -p $RPM_BUILD_ROOT$dir
		fi
	chown $USER_PPR $RPM_BUILD_ROOT$dir 2>/dev/null &&
		chgrp $GROUP_PPR $RPM_BUILD_ROOT$dir 2>/dev/null
	chmod 755 $RPM_BUILD_ROOT$dir || exit 1
	done 

echo "%attr(-,$USER_PPR,$GROUP_PPR) %dir \"$LIBDIR\"">>$fileslist
echo "%attr(-,$USER_PPR,$GROUP_PPR) %dir \"$SHAREDIR\"">>$fileslist
echo "%attr(-,$USER_PPR,$GROUP_PPR) %dir \"$CONFDIR\"">>$fileslist
echo "%attr(-,$USER_PPR,$GROUP_PPR) %dir \"$VAR_SPOOL_PPR\"">>$fileslist

#
# We have to be more careful with this one since it is probably the 
# system-wide temporary directory and we don't want to mess up its
# permissions.
#
# Yes, the test does check the real system, not the $RPM_BUILD_ROOT!
#
if [ ! -d $TEMPDIR ]
	then
	mkdir -p $RPM_BUILD_ROOT$TEMPDIR 
	chown $USER_PPR $RPM_BUILD_ROOT$TEMPDIR
	chgrp $GROUP_PPR $RPM_BUILD_ROOT$TEMPDIR
	chmod 755 $RPM_BUILD_ROOT$dir || exit 1
	echo "%attr(-,$USER_PPR,$GROUP_PPR) %dir \"$TEMPDIR\"">>$fileslist
	fi

# It is necessary to create empty configuration
# directories.
directory $CONFDIR/printers 755
directory $CONFDIR/groups 755
directory $CONFDIR/aliases 755
directory $CONFDIR/mounted 755
directory $CONFDIR/acl 755

# Make the directories for the resource store.  Notice
# that we do not make a directory for fonts since they
# are handled separately.
directory $RESOURCEDIR 755
directory $RESOURCEDIR/procset 755
directory $RESOURCEDIR/file 755
directory $RESOURCEDIR/encoding 755

# Make the miscelaineous directories in /usr/lib/ppr.
directory $BINDIR 755
directory $FILTDIR 755
directory $INTDIR 755
directory $RESPONDERDIR 755
directory $LIBDIR/browsers 755
directory $LIBDIR/fixup 755
directory $LIBDIR/editps 755

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
directory $VAR_SPOOL_PPR/logs 775		# <-- group can write
directory $VAR_SPOOL_PPR/cache 755
directory $VAR_SPOOL_PPR/cache/font 755
directory $VAR_SPOOL_PPR/cache/procset 755
# The next one is commented out because to many drivers are too ill-behaved
# and generate multiple files with the same name.
#directory $VAR_SPOOL_PPR/cache/file
directory $VAR_SPOOL_PPR/cache/encoding 755
directory $VAR_SPOOL_PPR/dvips 755
directory $VAR_SPOOL_PPR/drivers 755
directory $VAR_SPOOL_PPR/drivers/W32X86 755		# MS-Windows 95/98
directory $VAR_SPOOL_PPR/drivers/WIN40 755		# MS-Windows NT 4.0
directory $VAR_SPOOL_PPR/drivers/WINPPD 755		# PPD files in MS-DOS text format
directory $VAR_SPOOL_PPR/drivers/macos 755
directory $VAR_SPOOL_PPR/sambaspool 1777
directory $VAR_SPOOL_PPR/pprclipr 755
directory $VAR_SPOOL_PPR/pprpopup.db 770		# <-- group can write
directory $VAR_SPOOL_PPR/followme.db 770		# <-- group can write

# Make the directories for web documentation and managment tools
directory $CGI_BIN 755
directory $SHAREDIR/www 755

exit 0
