#
# mouse:~ppr/src/makeprogs/installconf.sh
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
# Last modified 3 August 2003.
#

#
# This script is used in Makefiles set the permissions on config files and
# add them to the installed files list.	 Unlike the other install*.sh
# scripts, this one does not actually copy files.
#

. `dirname $0`/paths.sh

TYPE="$1"
shift

if [ "$TYPE" = "" ]
	then
	echo "Usage: installconf.sh <type> [<file1> ...]"
	exit 1
	fi

if [ -n "$RPM_BUILD_ROOT" -a ! -d "$RPM_BUILD_ROOT" ]
  then
  echo "The RPM_BUILD_ROOT \"$RPM_BUILD_ROOT\" does not exist!"
  exit 1
  fi

while [ "$1" != "" ]
	do
	if [ "$name" != "CVS" ]
		then
		if [ ! -f "$RPM_BUILD_ROOT$1" ]
			then
			echo "$1 doesn't exist!"
			exit 1
			fi

		# This may fail during RPM build.
		echo "  chown $USER_PPR:$GROUP_PPR \"$1\""
		chown $USER_PPR "$RPM_BUILD_ROOT$1" 2>/dev/null \
			&& chgrp $GROUP_PPR "$RPM_BUILD_ROOT$1" 2>/dev/null

		echo "  chmod 644 \"$1\""
		chmod 644 "$RPM_BUILD_ROOT$1" || exit 1

		# Mustn't use quote marks around the file name, rpmbuild doesn't like it!
		echo "%$TYPE \"$1\"" >>`dirname $0`/../z_install_begin/installed_files_list
		fi
	shift
	done

exit 0
