#! /bin/sh
#
# mouse:~ppr/src/po/install_mo.sh
# Copyright 1995--2004, Trinity College Computing Center.
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
# Last modified 15 April 2004.
#

#
# This script takes the translation files (.po files) and converts
# them to index, machine dependent .mo files and installs them
# in the locale directory.
#
# BUG: doesn't set correct permissions if run as root!
# BUG: uses echo -n
#

. `dirname $0`/../makeprogs/paths.sh

lang="$1"
instdir="$2"
fileslist="../z_install_begin/installed_files_list"

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
chown $PPR_USER "$full_instdir/$lang"
chgrp $PPR_GROUP "$full_instdir/$lang"
echo "%dir \"$instdir/$lang\"" >>$fileslist

if [ ! -d "$full_instdir/$lang/LC_MESSAGES" ]
	then
	mkdir "$full_instdir/$lang/LC_MESSAGES"
	fi
chown $PPR_USER "$full_instdir/$lang/LC_MESSAGES"
chgrp $PPR_GROUP "$full_instdir/$lang/LC_MESSAGES"
echo "%dir \"$instdir/$lang/LC_MESSAGES\"" >>$fileslist

echo -n "    "
for potfile in *.pot
	do
	division=`basename $potfile .pot`
	if [ "$division" = "*" ]
	then
	echo "No .pot files!"
	exit 1
	fi
	if [ -f "$lang-$division.po" ]
		then
		echo -n " $division"
		msgfmt -o "$full_instdir/$lang/LC_MESSAGES/$division.mo" $lang-$division.po || exit 1
		chown $USER_PPR "$full_instdir/$lang/LC_MESSAGES/$division.mo" 2>/dev/null \
			&& chown $USER_PPR "$full_instdir/$lang/LC_MESSAGES/$division.mo" 2>/dev/null
		echo "\"$instdir/$lang/LC_MESSAGES/$division.mo\"" >>$fileslist
		else
		echo -n " $division(missing)"
		fi
	done
echo

exit 0

