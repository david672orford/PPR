#
# mouse:~ppr/src/z_install_end/install_sample_to_conf.sh
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
# Last modified 21 February 2003.
#

#CONFDIR="?"
#USER_PPR=?
#GROUP_PPR=?
. ../makeprogs/paths.sh

#===========================================================================
# Creating empty ACL files if they are absent
#===========================================================================
echo "Creating missing ACL files..."

for f in pprprox.allow ppop.allow ppad.allow ppuser.allow
    do
    echo "  $CONFDIR/acl/$f"
    if [ ! -f $CONFDIR/acl/$f ]
    	then
	echo "    creating"
    	touch $CONFDIR/acl/$f
	else
	echo "    exists"
    	fi
    done

#===========================================================================
# Make configuration files with sample versions
#===========================================================================
echo "Making missing configuration files from the samples..."

for f in ppr.conf uprint.conf uprint-remote.conf lprsrv.conf
    do
    echo "  $f"
    if [ ! -f $CONFDIR/$f -a -f $CONFDIR/$f.sample ]
    	then
	echo "    $CONFDIR/$f.sample --> $CONFDIR/$f"
	# Use Sed to copy it while removing the "Last modified" comment.
    	sed -e 's/\.sample$//' -e '/^[#;] Last modified/d' $CONFDIR/$f.sample >$CONFDIR/$f
    	chown $USER_PPR $CONFDIR/$f
    	chgrp $GROUP_PPR $CONFDIR/$f
	chmod 644 $CONFDIR/$f
	else
	echo "    exists"
    	fi
    done

exit 0
