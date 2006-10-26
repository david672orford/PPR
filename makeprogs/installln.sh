#! /bin/sh
#
# mouse:~ppr/src/makeprogs/installln.sh
# Copyright 1995--2006, Trinity College Computing Center.
# Written by David Chappell.
#
# This file is part of PPR.  You can redistribute it and modify it under the
# terms of the revised BSD licence (without the advertising clause) as
# described in the accompanying file LICENSE.txt.
#
# Last modified 26 October 2006.
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

# Do not do this!  It will change the permissions of the file to which the
# link points rather than the permissions of the link itself.
#chown $USER_PPR "$RPM_BUILD_ROOT$target" 2>/dev/null
#chgrp $GROUP_PPR "$RPM_BUILD_ROOT$target" 2>/dev/null

echo "%attr(-,$USER_PPR,$GROUP_PPR) \"$target\"" >>$MYDIR/../z_install_begin/installed_files_list

exit 0
