#! /bin/sh
#
# mouse:~ppr/src/makeprogs/installprogs.sh
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
# Last modified 9 February 2001.
#

#
# This script is used in Makefiles to copy programs (both binary and executable
# scripts) into place.  It is not used for Perl library or module (*.pl and
# *.pm) files.
#

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
    name=`basename "$1"`
    dest="$DESTDIR/$name"
    echo "    \"$1\" --> \"$RPM_BUILD_ROOT$dest\""
    rm -f "$RPM_BUILD_ROOT$dest" || exit 1
    cp "$1" "$RPM_BUILD_ROOT$dest" || exit 1
    strip "$RPM_BUILD_ROOT$dest" 2>/dev/null
    chown $USER "$RPM_BUILD_ROOT$dest" 2>/dev/null
    chgrp $GROUP "$RPM_BUILD_ROOT$dest" 2>/dev/null
    chmod $MODE "$RPM_BUILD_ROOT$dest" || exit 1
    echo "\"$dest\"" >>`dirname $0`/installed_files_list
    shift
    done

exit 0

