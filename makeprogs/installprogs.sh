#! /bin/sh
#
# mouse:~ppr/src/makeprogs/installprogs.sh
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
# Last modified 29 July 2003.
#

#
# This script is used in Makefiles to copy programs (both binary and executable
# scripts) into place.	It is not used for Perl library or module (*.pl and
# *.pm) files.
#

MYDIR=`dirname $0`

USER=$1
GROUP=$2
MODE=$3
DESTDIR="$4"
shift 4

if [ "$DESTDIR" = "" ]
  then
  echo "Usage: installscripts.sh <user> <group> <mode> <destdir> [<script1> ...]"
  exit 1
  fi

if [ -n "$RPM_BUILD_ROOT" -a ! -d "$RPM_BUILD_ROOT" ]
  then
  echo "The RPM_BUILD_ROOT \"$RPM_BUILD_ROOT\" does not exist!"
  exit 1
  fi

if [ ! -d "$RPM_BUILD_ROOT$DESTDIR" ]
  then
  echo "The destination directory \"$RPM_BUILD_ROOT$DESTDIR\" doesn't exist."
  exit 1
  fi

while [ "$1" != "" ]
	do
	file=$1
	shift
	name=`basename "$file"`
	dest="$DESTDIR/$name"
	echo "    \"$file\" --> \"$RPM_BUILD_ROOT$dest\""

	#
	# If this file is to be owned by root, try very hard not to replace
	# it unless is has really changed.	We do this so that make install
	# may be run by a non-root user after it has been run once by root.
	#
	if [ "$USER" = "root" -a -f "$RPM_BUILD_ROOT$dest" -a ! -w "$RPM_BUILD_ROOT$dest" ]
		then
		strip $file 2>/dev/null
		if diff $file "$RPM_BUILD_ROOT$dest" >/dev/null 2>&1
			then
			echo "        (skipping copy because root ownership and unchanged)"
			continue
			fi
		fi

    #
    # Remove the original file, copy the new one into place, and strip it.
    # We explicitly remove the original because it may be running.  If it
    # is, the OS either won't let use open it for write (text busy) or 
    # writing over it will cause the running program to crash.
    #
	rm -f "$RPM_BUILD_ROOT$dest" || exit 1
	cp "$file" "$RPM_BUILD_ROOT$dest" || exit 1
	strip "$RPM_BUILD_ROOT$dest" 2>/dev/null

	#
	# Here we try to set the file permissions.  If this doesn't work, they try
	# to leave the command to be executed by root later.
	#
	chown $USER "$RPM_BUILD_ROOT$dest" \
		&& chgrp $GROUP "$RPM_BUILD_ROOT$dest" \
		&& chmod $MODE "$RPM_BUILD_ROOT$dest"
	if [ $? -ne 0 ]
		then
		if [ -f $MYDIR/../root.sh ]
			then
			echo "Writing program permissions commands to $MYDIR/../root.sh."
			echo "chown $USER \"$RPM_BUILD_ROOT$dest\" && chgrp $GROUP \"$RPM_BUILD_ROOT$dest\" && chmod $MODE \"$RPM_BUILD_ROOT$dest\" || exit $?" \
				>>$MYDIR/../root.sh
			else
			echo "============================================================================="
			echo " The above error almost certainly means that you must re-run make install as"
			echo " root (as least in this one directory).  This will happen the first time and"
			echo " every time you modify a setuid root program."
			echo "============================================================================="
			exit 1
			fi
		fi

	echo "\"$dest\"" >>`dirname $0`/../z_install_begin/installed_files_list

	done

exit 0

