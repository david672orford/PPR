#! /bin/sh
#
# mouse:~ppr/src/makeprogs/installcp.sh
# Copyright 1995--2001, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software is provided "as is" without express or
# implied warranty.
#
# Last modified 28 March 2001.
#

#
# This script copies a single file into place.
#

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
    echo "    \"$source\" --> \"$RPM_BUILD_ROOT$target\" (squeezing)"
    `dirname $0`/squeeze "$source" "$RPM_BUILD_ROOT$target" || exit 1
    else
    echo "    \"$source\" --> \"$RPM_BUILD_ROOT$target\""
    cp "$source" "$RPM_BUILD_ROOT$target" || exit 1
    chmod 644 "$RPM_BUILD_ROOT$target" || exit 1
    fi

echo "\"$target\"" >>`dirname $0`/installed_files_list

exit 0

