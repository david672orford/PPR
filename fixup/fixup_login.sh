#
# mouse:~ppr/src/fixup/fixup_login.sh
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
# Last modified 28 January 2002.
#

HOMEDIR="?"
PROFILE_D="/etc/profile.d"

#======================================================================
# Add shell startup files to select PPR responders.
# This will soon be obsolete.
#======================================================================

# Formerly we kept these files in $HOMEDIR/lib and put symbolic links in 
# /etc/profile.d, but it doesn't seem a good idea to have ppr writable login
# files in a place where root will execute them.
if [ -d $PROFILE_D ]
    then
    echo "Copying login files to \"$PROFILE_D\"..."
    for i in login_ppr.sh login_ppr.csh
        do
        echo "    cp $HOMEDIR/fixup/$i $PROFILE_D/$i"
        rm -f $PROFILE_D/$i
        cp $HOMEDIR/fixup/$i $PROFILE_D/$i || exit 1
	chown root $PROFILE_D/$i
	chmod 555 $PROFILE_D/$i
        done
    echo "Done."
    echo
    fi

exit 0

