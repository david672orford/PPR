#! /bin/sh
#
# mouse:~ppr/src/po/merge_to_pox.sh
# Copyright 1995--1999, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is" without
# express or implied warranty.
#
# Last modified 29 December 1999.
#

#
# This program takes the current .pot files (translation string template
# files) and uses them to update the .po files, making temporary .po
# files called .pox files.
#

lang=$1
if [ -z "$lang" ]
	then
	echo "Usage: install_mo.sh <language>"
	exit 1
	fi

for potfile in *.pot
	do
	division=`basename $potfile .pot`
	echo -n "$lang-$division.pox: "
	if [ -f "$lang-$division.po" ]
	then
	echo "merging old and new messages"
	#tupdate $division.pot $lang-$division.po >$lang-$division.pox
	msgmerge $lang-$division.po $division.pot >$lang-$division.pox
	else
	echo "creating new"
	cp $division.pot $lang-$division.pox
	fi
	done
echo

exit 0

