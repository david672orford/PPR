#! /bin/sh
#
# mouse:~ppr/src/po/install_mo.sh
# Copyright 1995--2001, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is" without
# express or implied warranty.
#
# Last modified 31 July 2001.
#

#
# This script takes the translation files (.po files) and converts
# them to index, machine dependent .mo files and installs them
# in the locale directory.
#

lang="$1"
instdir="$2"
fileslist="../makeprogs/installed_files_list"

if [ -z "$instdir" -o -z "$lang" ]
    then
    echo "Usage: install_mo.sh <language> <locale_directory>"
    exit 1
    fi

if [ -n "$RPM_BUILD_ROOT" -a ! -d "$RPM_BUILD_ROOT" ]
  then
  echo "The RPM_BUILD_ROOT \"$RPM_BUILD_ROOT\" does not exist!"
  exit 1
  fi

full_instdir="$RPM_BUILD_ROOT$instdir"

if [ ! -d "$full_instdir" ]
    then
    echo "The directory \"$full_instdir\" does not exist."
    exit 1
    fi

echo "Installing language \"$lang\" in \"$full_instdir\"."

# Create the directory to hold the translation files for this language.
# the top level locale directory should have been already created.
if [ ! -d "$full_instdir/$lang" ]
    then
    mkdir "$full_instdir/$lang"
    fi
echo "%dir \"$instdir/$lang\"" >>$fileslist
if [ ! -d "$full_instdir/$lang/LC_MESSAGES" ]
    then
    mkdir "$full_instdir/$lang/LC_MESSAGES"
    fi
echo "%dir \"$instdir/$lang/LC_MESSAGES\"" >>$fileslist

echo -n "    "
for potfile in *.pot
    do
    division=`echo $potfile | cut -d. -f1`
    if [ -f "$lang-$division.po" ]
        then
        echo -n " $division"
        msgfmt $lang-$division.po -o "$full_instdir/$lang/LC_MESSAGES/$division.mo" || exit 1
	echo "\"$instdir/$lang/LC_MESSAGES/$division.mo\"" >>$fileslist
        else
        echo -n " $division(missing)"
        fi
    done
echo

exit 0

