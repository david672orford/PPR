#! /bin/sh
#
# mouse:~ppr/src/makeprogs/make_new_dir.sh
# Copyright 1995--2000, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appears in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is"
# without express or implied warranty.
#
# Last modified 9 February 2001.
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

echo "%dir \"$dir\"" >>`dirname $0`/installed_files_list

exit 0
