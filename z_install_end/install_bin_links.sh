#
# mouse:~ppr/src/z_install_end/install_bin_links.sh
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
# Last modified 24 May 2004.
#

. ../makeprogs/paths.sh

#======================================================================
# Create links so that the PPR bin directory need
# not be in the PATH.
#======================================================================

echo "Creating symbolic links in \"$SYSBINDIR\"..."
if [ ! -d $RPM_BUILD_ROOT$SYSBINDIR ]
    then
	mkdir -p $RPM_BUILD_ROOT$SYSBINDIR || exit 1
    fi
for i in ppr ppop ppad ppuser ppdoc \
	ppr-testpage \
	ppr-config \
	ppr-followme ppr-xgrant ppr-popup \
	ppr-panel ppr-web ppr-passwd
	do
	../makeprogs/installln.sh $HOMEDIR/bin/$i $SYSBINDIR/$i || exit 1
	done

exit 0
