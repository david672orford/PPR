#! /bin/sh
#
# mouse:~ppr/src/makeprogs/make_new_dir.sh
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
# Last modified 5 April 2003.
#

#
# This script removes an installation directory and its old contents
# and creates a new one.
#

# Pull in PPR paths and user names.
. `dirname $0`/paths.sh

dir="$1"

if [ -z "$dir" ]
	then
	echo "Usage: make_new_dir.sh <dirname>"
	exit 1
	fi

if [ -n "$RPM_BUILD_ROOT" -a ! -d "$RPM_BUILD_ROOT" ]
  then
  echo "The RPM_BUILD_ROOT \"$RPM_BUILD_ROOT\" does not exist!"
  exit 1
  fi

if [ -d "$RPM_BUILD_ROOT$dir" ]
	then
	echo "    Removing old directory \"$RPM_BUILD_ROOT$dir\"..."
	rm -rf "$RPM_BUILD_ROOT$dir"
	fi

if [ -d "$RPM_BUILD_ROOT$dir" ]
	then
	echo "Failed to remove \"$RPM_BUILD_ROOT$dir\"."
	exit 1
	fi

echo "    Creating new directory \"$RPM_BUILD_ROOT$dir\"..."
mkdir "$RPM_BUILD_ROOT$dir" || exit 1
chown $USER_PPR "$RPM_BUILD_ROOT$dir" 2>/dev/null
chgrp $GROUP_PPR "$RPM_BUILD_ROOT$dir" 2>/dev/null
chmod 755 "$RPM_BUILD_ROOT$dir"

echo "%dir \"$dir\"" >>`dirname $0`/../z_install_begin/installed_files_list

exit 0
