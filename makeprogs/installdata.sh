#
# mouse:~ppr/src/installdata.sh
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
# Last modified 13 September 2000.
#

#
# This script is used in Makefiles to copy data files into place.  Perl library
# and module (*.pl and *.pm) files are considered data for the purposes of this
# script.
#

DESTDIR="$1"
shift

if [ "$DESTDIR" = "" ]
    then
    echo "Usage: installdata.sh <destdir> [<file1> ...]"
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
    echo "\"$dest\"" >>`dirname $0`/installed_files_list
    shift
    done

exit 0
