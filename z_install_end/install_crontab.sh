#! /bin/sh
#
# mouse:~ppr/src/z_install_end/install_crontab.sh
# Copyright 1995--2003, Trinity College Computing Center.
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
# Last modified 5 March 2003.
#

. ../makeprogs/paths.sh

./puts "Making sure we are $USER_PPR..."
if [ "`../z_install_begin/id -un`" = "$USER_PPR" ]
    then
    echo " OK"
    else
    echo " Nope, we must be root, doing su..."
    su $USER_PPR -c $0    
    exit $?
    fi

echo "Installing crontab for the user $USER_PPR..."

# Create a temporary file.  We have to do this because of limitations
# of some versions of the crontab program.
# Sadly, not all versions of crontab can read from stdin, at least not
# with the same command.
tempname=`$HOMEDIR/lib/mkstemp $TEMPDIR/ppr-fixup_cron-XXXXXX`

# Write the new crontab to a temporary file.
cat - >$tempname <<===EndOfHere===
3 10,16 * * 1-5 $HOMEDIR/bin/ppad remind
5 4 * * * $HOMEDIR/lib/cron_daily
17 * * * * $HOMEDIR/lib/cron_hourly
===EndOfHere===

crontab $tempname

rm $tempname

exit 0

