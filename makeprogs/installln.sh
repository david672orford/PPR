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
# Last modified 3 August 2003.
#

#
# This script is used to install links to programs.	 If the link is already
# ok, we don't remove it.  This allows us to run to completion even if we
# wouldn't be able to modify the link.	This allows us to do make install
# as the ppr user if it has previously been done as root.
#

MYDIR=`dirname $0`

. $MYDIR/paths.sh

READLINK=$MYDIR/readlink

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

echo "    \"$source\" --> \"$target\" (link)"

if [ `echo $source | cut -c1` != / ]
	then
	source_verify="`dirname $target`/$source"
	else
	source_verify="$source"
	fi
if [ ! -f $source_verify -a ! -f "$RPM_BUILD_ROOT$source_verify" ]
	then
	echo "The source file \"$source_verify\" doesn't exist."
	exit 1
	fi

if [ "`$READLINK $RPM_BUILD_ROOT$target`" != "$source" ]
	then
	rm -f "$RPM_BUILD_ROOT$target"
	ln -s "$source" "$RPM_BUILD_ROOT$target"
	if [ $? -ne 0 ]
		then
		if [ -f $MYDIR/../root.sh ]
			then
			echo "    Writing link installation command to $MYDIR/../root.sh."
			echo "rm -f \"$RPM_BUILD_ROOT$target\"; ln -s \"$source\" \"$RPM_BUILD_ROOT$target\"" >>$MYDIR/../root.sh
			else
			echo "============================================================================="
			echo " The above error almost certainly means that you must re-run make install as"
			echo " root (as least in this one directory).  This will happen every time a link"
			echo " in a system directory must be changed."
			echo "============================================================================="
			exit 1
			fi
		fi
	fi
#ls -l $RPM_BUILD_ROOT$target
chown $USER_PPR "$RPM_BUILD_ROOT$target" 2>/dev/null
chgrp $GROUP_PPR "$RPM_BUILD_ROOT$target" 2>/dev/null

echo "\"$target\"" >>$MYDIR/../z_install_begin/installed_files_list

exit 0
