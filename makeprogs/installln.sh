#! /bin/sh
#
# mouse:~ppr/src/makeprogs/installln.sh
# Copyright 1995--2000, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software is provided "as is" without express or
# implied warranty.
#
# Last modified 13 September 2000.
#

#
# This script is used to install links to other installed programs.
#

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

rm -f "$RPM_BUILD_ROOT$target"
echo "    \"$RPM_BUILD_ROOT$source\" --> \"$RPM_BUILD_ROOT$target\" (link)"
ln "$RPM_BUILD_ROOT$source" "$RPM_BUILD_ROOT$target" || exit 1

echo "\"$target\"" >>`dirname $0`/installed_files_list

exit 0

