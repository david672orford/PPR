#
# mouse:~ppr/src/z_install_begin/install_create_accounts.sh
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
# Last modified 5 August 2003.
#

# The makefile has to feed us this information since ../makeprogs/paths.sh
# doesn't exist yet.
HOMEDIR=$1
USER_PPR=$2
USER_PPRWWW=$3
GROUP_PPR=$4
COMMENT="PPR Spooling System"

my_uid=`./id -u`
my_gid=`./id -g`

echo "uid=$my_uid gid=$my_gid"

#=============================================================================
# Make sure there is a group called "ppr".
#=============================================================================

echo "Checking for group $GROUP_PPR..."
group_ppr_gid=`./getgrnam $GROUP_PPR`
if [ $group_ppr_gid -ne -1 ]
	then
	echo "    already exists, good."

	else
	echo "    adding..."

	if [ $my_uid -ne 0 ]
		then
		echo "Can't create group $GROUP_PPR because not running as root."
		exit 1
		fi

	if [ -x /usr/sbin/groupadd ]
		then
		/usr/sbin/groupadd $GROUP_PPR
		else
		if [ -x /usr/sbin/pw ]
			then
			/usr/sbin/pw groupadd $GROUP_PPR
			else
			if [ -x /usr/bin/niload ]
				then
				echo "$GROUP_PPR:*:100:" | niload group .
				else
				echo "Oops, this script doesn't contain instructions for adding groups on this system."
				echo "Please add the group \"$GROUP_PPR\" and then run fixup again."
				exit 1
				fi
			fi
		fi

	group_ppr_gid=`./getgrnam $GROUP_PPR`
	if [ $group_ppr_gid -lt 1 ]
		then
		echo
		echo "Your groupadd or pw command is broken.  Please edit /etc/group, and create"
		echo "an entry for the group \"$GROUP_PPR\" with an ID greater than 0."
		echo
		exit 1
		fi
	fi

#=======================================================================
# Make sure the users ppr and pprwww exist.
#=======================================================================

for user in $USER_PPR $USER_PPRWWW
	do
	if [ $user = $USER_PPR ]
	then
	sup="-G lp"
	else
	sup=""
	fi

	echo "Checking for user $user..."
	user_ppr_uid=`./getpwnam $user`
	if [ $user_ppr_uid -ne -1 ]
		then
		echo "    already exists, good."

		# doesn't exist
		else
		echo "    adding..."

	if [ $my_uid -ne 0 ]
		then
		echo "Can't create user $GROUP_PPR because not running as root."
		exit 1
		fi

	# System V release 4
	if [ -x /usr/sbin/passmgmt ]
		then
		/usr/sbin/passmgmt -a -h $HOMEDIR -c "$COMMENT" -g $group_ppr_gid $user
		exitval=$?

	# Linux
	else
	if [ -x /usr/sbin/useradd -a `uname -s` = "Linux" ]
		then
		/usr/sbin/useradd -M -d $HOMEDIR -c "$COMMENT" -g $group_ppr_gid $sup $user
		exitval=$?

	# OSF?
	else
	if [ -x /usr/sbin/useradd ]
		then
		/usr/sbin/useradd -d $HOMEDIR -c "$COMMENT" -g $group_ppr_gid $user
		exitval=$?

	# FreeBSD
	else
	if [ -x /usr/sbin/pw ]
		then
		/usr/sbin/pw useradd -d $HOMEDIR -c "$COMMENT" -g $group_ppr_gid $user
		exitval=$?

	# MacOS X
	else
	if [ -x /usr/bin/niload ]
		then
		if [ "$user" = "$USER_PPR" ]
		then
		uid=100
		else
		uid=101
		fi
		echo "$user:*:$uid:$group_ppr_gid:0:0::$COMMENT:$HOMEDIR:" | niload passwd .
		exitval=$?

	else				# no user passmgmt or useradd
	echo
	echo "Oops!  This script does not contains instructions for adding accounts on"
	echo "this system, please add an account called \"$user\" and run make install"
	echo "again.  The account should have a primary group ID of $group_ppr_gid."
	exit 1
		fi
		fi
	fi
	fi
	fi

		# did passmgmt or useradd work?
		if [ $exitval -ne 0 ]		# if error occured, stop the script
			then
			exit 1
			fi

		fi
	done

#=======================================================================
#
#=======================================================================

user_ppr_uid=`./getpwnam $USER_PPR`
echo "user_ppr_uid=$user_ppr_uid group_ppr_gid=$group_ppr_gid"

if [ $my_uid != 0 -a $my_uid != $user_ppr_uid ]
	then
	echo "You aren't root or $USER_PPR.  You won't be able to install PPR"
	echo "with the correct file ownership.  Aborting."
	fi

if [ $my_uid != 0 -a $my_gid != $group_ppr_gid ]
	then
	echo "Your primary group isn't $GROUP_PPR.  You won't be able to install PPR"
	echo "with the correct file ownership.  Aborting."
	fi

exit 0
