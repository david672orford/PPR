#
# mouse:~ppr/src/z_install_end/install_sample_to_conf.sh
# Copyright 1995--2006, Trinity College Computing Center.
# Written by David Chappell.
#
# This file is part of PPR.  You can redistribute it and modify it under the
# terms of the revised BSD licence (without the advertising clause) as
# described in the accompanying file LICENSE.txt.
#
# Last modified 14 December 2006.
#

. ../makeprogs/paths.sh

#===========================================================================
# Creating empty ACL files if they are absent
#===========================================================================
echo "Creating missing ACL files..."

for f in pprprox.allow ppop.allow ppad.allow ppuser.allow
	do
	./puts "  $CONFDIR/acl/$f..."
	if [ ! -f $RPM_BUILD_ROOT$CONFDIR/acl/$f ]
		then
		echo " creating"
		touch $RPM_BUILD_ROOT$CONFDIR/acl/$f
	else
		echo " exists"
	fi
	../makeprogs/installconf.sh $USER_PPR $GROUP_PPR 644 'config(noreplace)' $CONFDIR/acl/$f
	done

#===========================================================================
# Make configuration files with sample versions
#===========================================================================
echo "Making missing configuration files from the samples..."

for f in ppr.conf lprsrv.conf
	do
	./puts "  $f..."
	if [ ! -f $RPM_BUILD_ROOT$CONFDIR/$f ]
		then
		echo " copy $CONFDIR/$f.sample --> $CONFDIR/$f"
		# Use Sed to copy it while removing the "Last modified" comment.
		sed -e 's/\.sample$//' -e '/^[#;] Last modified/d' $RPM_BUILD_ROOT$CONFDIR/$f.sample >$RPM_BUILD_ROOT$CONFDIR/$f
		else
		echo " exists"
		fi
	# Using 'config(noreplace)' in place of 'config' means that RPM will write the
	# new config files as .rpmnew and leave the old ones in place rather than
	# moving the old ones to .rpmsave and installing the new ones.
	../makeprogs/installconf.sh $USER_PPR $GROUP_PPR 644 'config(noreplace)' $CONFDIR/$f
	done

exit 0
