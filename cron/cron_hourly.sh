#! /bin/sh
#
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
# Last modified 4 March 2003.
#

HOMEDIR="?"
VAR_SPOOL_PPR="?"

# This is a list of files which might be lists of installed packages.  The
# file /var/lib/rpm/packages.rpm is for Red Hat Linux's RPM format, the file
# /var/sadm/install/contents is for Solaris's package format, the file
# /var/lib/dpkg/status is for Debian GNU/Linux
PACKAGE_LISTS="/var/lib/rpm/packages.rpm /var/sadm/install/contents /var/lib/dpkg/status"

#
# If any of the package lists has changed since the font index was
# generated, rebuild the font index now.
#
for i in $PACKAGE_LISTS
    do
    if [ -f $i ]
	then
	if $HOMEDIR/lib/file_outdated $VAR_SPOOL_PPR/fontindex.db $i
	    then
	    $HOMEDIR/bin/ppr-indexfonts >$VAR_SPOOL_PPR/logs/ppr-indexfonts 2>&1
	    fi
	if $HOMEDIR/lib/file_outdated $VAR_SPOOL_PPR/ppdindex.db $i
	    then
	    $HOMEDIR/bin/ppr-indexppds >$VAR_SPOOL_PPR/logs/ppr-indexppds 2>&1
	    fi
	fi
    done

exit 0
