#
# mouse:~ppr/src/fixup/fixup_samples.sh
# Copyright 1995--2002, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appears in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is"
# without express or implied warranty.
#
# Last modified 23 April 2002.
#

CONFDIR="?"
EECHO="?"
USER_PPR=?
GROUP_PPR=?

#===========================================================================
# Creating empty ACL files if they are absent
#===========================================================================
echo "Creating missing ACL files..."

for f in pprprox.allow ppop.allow ppad.allow ppuser.allow
    do
    if [ ! -f $CONFDIR/acl/$f ]
    	then
	$EECHO "\t$CONFDIR/acl/$f"
    	touch $CONFDIR/acl/$f
    	fi
    done
echo "Done."
echo

#===========================================================================
# Make configuration files with sample versions
#===========================================================================
echo "Making missing configuration files from the samples..."

for f in ppr.conf uprint.conf uprint-remote.conf lprsrv.conf
    do
    if [ ! -f $CONFDIR/$f -a -f $CONFDIR/$f.sample ]
    	then
	echo $CONFDIR/$f
	# Use Sed to copy it while removing the "Last modified" comment.
    	sed -e 's/\.sample$//' -e '/^[#;] Last modified/d' $CONFDIR/$f.sample >$CONFDIR/$f
    	chown $USER_PPR $CONFDIR/$f
    	chgrp $GROUP_PPR $CONFDIR/$f
	chmod 644 $CONFDIR/$f
    	fi
    done
echo "Done."
echo

exit 0

