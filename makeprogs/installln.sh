#! /bin/sh
#
# mouse:~ppr/src/makeprogs/installln.sh
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
# Last modified 7 March 2003.
#

#
# This script is used to install links to programs.  If the link is already
# ok, we don't remove it.  This allows us to run to completion even if we
# wouldn't be able to modify the link.  This allows us to do make install
# as the ppr user if it has previously been done as root.
#

. `dirname $0`/paths.sh

if [ -z "$1" -o -z "$2" ]
  then
  echo "Usage: installln.sh: <sourcefile> <destfile>"
  exit 1
  fi

if [ -n "$RPM_BUILD_ROOT" -a ! -d "$RPM_BUILD_ROOT" ]
  then
  echo "The RPM_BUILD_ROOT \"$RPM_BUILD_ROOT\" does not exist!"
  exit 1
  fi

source="$1"
target="$2"

if [ ! -f "$RPM_BUILD_ROOT$source" ]
  then
  echo "The source file \"$RPM_BUILD_ROOT$source\" doesn't exist."
  exit 1
  fi

echo "    \"$source\" --> \"$target\" (link)"
if [ "`readlink $RPM_BUILD_ROOT$target`" != "$source" ]
    then
    rm -f "$RPM_BUILD_ROOT$target"
    ln -s "$RPM_BUILD_ROOT$source" "$RPM_BUILD_ROOT$target"
    if [ $? -ne 0 ]
	then
	echo "$0: can't create \"$target\" because not running as root."
	exit 1
	fi
    fi
chown $USER_PPR $RPM_BUILD_ROOT$target 2>/dev/null
chgrp $GROUP_PPR $RPM_BUILD_ROOT$target 2>/dev/null

echo "\"$target\"" >>`dirname $0`/../z_install_begin/installed_files_list

exit 0

