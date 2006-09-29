#! /bin/sh
#
# mouse:~ppr/src/z_install_end/install_crontab.sh
# Copyright 1995--2006, Trinity College Computing Center.
# Written by David Chappell.
#
# This file is part of PPR.  You can redistribute it and modify it under the
# terms of the revised BSD licence (without the advertising clause) as
# described in the accompanying file LICENSE.txt.
#
# Last modified 17 August 2006.
#

. ../makeprogs/paths.sh

# Sanity test gauntet
if [ -z "$USER_PPR" ]
	then
	echo "$0: ../makeprogs/paths.sh is bad"
	exit 1
	fi
if [ ! -x ../z_install_begin/id ]
	then
	echo "$0: ../z_install_begin/id doesn't exist"
	exit 1
	fi

./puts "  Making sure we are $USER_PPR..."
if [ "`../z_install_begin/id -un`" = "$USER_PPR" ]
	then
	echo "    OK"
	else
	echo "    Nope, guess we are root, doing su $USER_PPR..."
	echo "su $USER_PPR -c $0"
	su $USER_PPR -c $0
	su_exit=$?
	if [ $su_exit -ne 0 ]
		then
		echo "Failed."
		fi
	exit $su_exit
	fi

echo "  Installing crontab for the user $USER_PPR..."

#
# Create a version of the crontab with the username.
#
# We must use a temporary file because of limitations of some
# versions of the crontab program.  Sadly, not all versions of crontab can 
# read from stdin, at least not all with the same command.
#
tempname=`$LIBDIR/mkstemp $TEMPDIR/ppr-fixup_cron-XXXXXX`
awk '{ print $1,$2,$3,$4,$5,$7,$8 }' <cron.d >$tempname
crontab $tempname
rm $tempname

exit 0

