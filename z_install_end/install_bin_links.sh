#
# mouse:~ppr/src/z_install_end/install_bin_links.sh
# Copyright 1995--2006, Trinity College Computing Center.
# Written by David Chappell.
#
# This file is part of PPR.  You can redistribute it and modify it under the
# terms of the revised BSD licence (without the advertising clause) as
# described in the accompanying file LICENSE.txt.
#
# Last modified 26 October 2006.
#

#======================================================================
# Create links so that the PPR bin directory need
# not be in the PATH.
#======================================================================

echo "Creating symbolic links in \"$SYSBINDIR\"..."

test -d $RPM_BUILD_ROOT$SYSBINDIR || mkdir -p $RPM_BUILD_ROOT$SYSBINDIR || exit 1

for i in ppr ppop ppad ppuser ppdoc \
		ppr-testpage \
		ppr-config ppr-index \
		ppr-followme ppr-xgrant \
		ppr-panel ppr-web ppr-passwd
	do
	../makeprogs/installln.sh $BINDIR/$i $SYSBINDIR/$i || exit 1
	done

exit 0
