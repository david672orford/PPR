#! /bin/sh
#
# mouse:~ppr/src/makeprogs/installcp.sh
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
# Last modified 5 August 2003.
#

#
# This script copies a single file into place.
#

. `dirname $0`/paths.sh

squeeze=0
if [ "$1" = "--squeeze" ]
  then
  squeeze=1
  shift
  fi

if [ -z "$1" -o -z "$2" ]
  then
  echo "Usage: installcp.sh: <sourcefile> <destfile>"
  exit 1
  fi

if [ -n "$RPM_BUILD_ROOT" -a ! -d "$RPM_BUILD_ROOT" ]
  then
  echo "The RPM_BUILD_ROOT \"$RPM_BUILD_ROOT\" does not exist!"
  exit 1
  fi

source="$1"
target="$2"

if [ ! -f "$source" ]
  then
  echo "The source file \"$source\" doesn't exist."
  exit 1
  fi

rm -f "$RPM_BUILD_ROOT$target"

if [ $squeeze != 0 ]
	then
	echo "    \"$source\" --> \"$target\" (squeezing)"
	`dirname $0`/squeeze "$source" "$RPM_BUILD_ROOT$target" || exit 1
	else
	echo "    \"$source\" --> \"$target\""
	cp "$source" "$RPM_BUILD_ROOT$target" || exit 1
	chown $USER_PPR "$RPM_BUILD_ROOT$target" 2>/dev/null \
		&& chgrp $GROUP_PPR "$RPM_BUILD_ROOT$target" 2>/dev/null
	chmod 644 "$RPM_BUILD_ROOT$target" || exit 1
	fi

echo "%attr(-,$USER_PPR,$GROUP_PPR) \"$target\"" >>`dirname $0`/../z_install_begin/installed_files_list

exit 0

