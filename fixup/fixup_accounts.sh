#
# mouse:~ppr/src/fixup/fixup_accounts.sh
# Copyright 1995--2000, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appears in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is"
# without express or implied warranty.
#
# Last modified 28 December 2000.
#

HOMEDIR="?"
GROUP_PPR=?
USER_PPR=?
USER_PPRWWW=?
NECHO="?"

echo "Creating accounts and groups for PPR."

#=============================================================================
# Make sure there is a group called "ppr".
#=============================================================================

$NECHO -n "    Group $GROUP_PPR: "
group_ppr_gid=`$HOMEDIR/lib/getgrnam $GROUP_PPR`
if [ $group_ppr_gid -ne -1 ]
    then
    echo "already exists, good."

    else
    echo "adding..."

    if [ -x /usr/sbin/groupadd ]
        then
        /usr/sbin/groupadd $GROUP_PPR
    else
    if [ -x /usr/sbin/pw ]
	then
	/usr/sbin/pw groupadd $GROUP_PPR
    else
        echo "Oops, this script doesn't contain instructions for adding groups on this system."
        echo "Please add the group \"$GROUP_PPR\" and then run fixup again."
        exit 1
    fi
    fi

    # Make sure it worked.  We look for a group id of 0 because Slackware
    # 2.x had a groupadd command which was broken.
    group_ppr_gid=`$HOMEDIR/lib/getgrnam $GROUP_PPR`
    if [ $group_ppr_gid -eq -1 -o $group_ppr_gid -eq 0 ]
        then
        echo
        echo "Your groupadd command is broken.  Please edit /etc/group, make sure there is"
        echo "a valid entry for the group \"$GROUP_PPR\" with an ID other than 0, and then"
        echo "run fixup again."
        echo
        exit 1
        fi
    fi

#=======================================================================
# We will add the account if this is a System V Release 4 system
# or other system which provides /usr/sbin/passmgmt, for other
# systems, we will let the user do it.
#=======================================================================

for user in $USER_PPR $USER_PPRWWW
    do
    $NECHO -n "    User $user: "
    user_ppr_uid=`$HOMEDIR/lib/getpwnam $user`
    if [ $user_ppr_uid -ne -1 ]
        then
        echo "already exists, good."

        # doesn't exist
        else
        echo "adding..."

	# System V release 4
        if [ -x /usr/sbin/passmgmt ]
            then
            /usr/sbin/passmgmt -a -h $HOMEDIR -c 'PPR Spooling System' -g $group_ppr_gid $user
            exitval=$?

	# Linux
        else
        if [ -x /usr/sbin/useradd -a `uname -s` = "Linux" ]
            then
            /usr/sbin/useradd -M -d $HOMEDIR -c 'PPR Spooling System' -g $group_ppr_gid $user
            exitval=$?

	# OSF?
	else
        if [ -x /usr/sbin/useradd ]
            then
            /usr/sbin/useradd -d $HOMEDIR -c 'PPR Spooling System' -g $group_ppr_gid $user
            exitval=$?

	# FreeBSD
	else
	if [ -x /usr/sbin/pw ]
	    then
	    /usr/sbin/pw useradd -d $HOMEDIR -c 'PPR Spooling System' -g $group_ppr_gid $user
	    exitval=$?

	else                # no user passmgmt or useradd
	echo
	echo "Oops!  This script does not contains instructions for"
	echo "adding accounts on this system, please add an account"
	echo "called \"$user\" and run this script again."
	exit 1
        fi
        fi
	fi
	fi

        # did passmgmt or useradd work?
        if [ $exitval -ne 0 ]       # if error occured, stop the script
            then
            exit 1
            fi

        fi
    done

echo "Done."
echo

exit 0

