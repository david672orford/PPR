#! /bin/sh
#
# mouse:~ppr/src/fixup/fixup_init.sh
# Copyright 1995--2001, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is" without
# express or implied warranty.
#
# Last modified 9 January 2001.
#

HOMEDIR="?"
CONFDIR="?"

#========================================================================
# Part 1, figure out what we have
#========================================================================

echo "Studying Init script system..."

# Figure out if we have a System V init and what directories it uses:
INIT_BASE=""
for i in /etc/rc.d /etc /sbin
    do
    if [ -d $i/init.d ]
	then
	INIT_BASE=$i
	echo "    System V style init scripts found in \"$INIT_BASE\"."
	break
	fi
    done

#
# If we have a System V style init, determine which one it is
# and come up with a list of links.
#
# More accuration information about run levels and the startup
# order is needed.  Please contribute a corrected INIT_LIST line
# for your system.
#
case "$INIT_BASE" in
	# RedHat Linux
	"/etc/rc.d" )
	    # Needs checking:
	    INIT_LIST="rc2.d/S80ppr rc3.d/S80ppr rc4.d/S80ppr rc5.d/S80ppr rc0.d/K40ppr rc1.d/K40ppr rc6.d/K40ppr"
	    ;;

	# Several systems
	"/etc" )
	    # Debian Linux
	    if [ -x /usr/bin/dpkg -a -x /usr/sbin/update-rc.d ]
	    then
	    # Needs checking:
	    INIT_LIST="rc2.d/S80ppr rc3.d/S80ppr rc4.d/S80ppr rc5.d/S80ppr rc0.d/K20ppr rc6.d/K20ppr"
	    else

	    # Solaris 2.x, Generic System V
	    INIT_LIST="rc2.d/S80ppr rc0.d/K20ppr"
	    fi
	    ;;

	# OSF/1 3.2
    	"/sbin" )
	    # Needs checking:
	    INIT_LIST="rc3.d/S65ppr rc0.d/K00ppr rc2.d/K00ppr"
	    ;;

	# No System V style init scripts
	"" )
	    ;;

	# No match
	* )
	    echo "Error in script, missing case!"
	    exit 1
	    ;;
	esac

#
# Look for rc.local which we will use on systems with no System V style
# startup scripts.  Also, earlier versions of PPR installed code in
# rc.local for systems for which it now installs System V style init
# scripts, so we want to know where this file is so we can warn the user if
# there is old PPR startup code lurking there.
#
RC_LOCAL=""
for i in /etc/rc.local /etc/rc.d/rc.local
    do
    if [ -f $i ]
        then
	echo "    BSD-style rc.local found at \"$i\"."
        RC_LOCAL=$i
        fi
    done

echo "Done studying."
echo

#========================================================================
# Part 2, install our code
#========================================================================

echo "Installing PPR startup code..."

#
# If we have a System V style init with System V style init scripts, install
# a startup script for PPR and make links in the directories for the
# appropriate run levels.
#
if [ -n "$INIT_BASE" ]
then
	echo "    Installing the System V style start and stop scripts:"

	# Install the principal script:
	echo "        $INIT_BASE/init.d/ppr"
	cp $HOMEDIR/fixup/init_ppr $INIT_BASE/init.d/ppr
	chown root $INIT_BASE/init.d/ppr
	chmod 755 $INIT_BASE/init.d/ppr

	# Remove any old links:
	rm -f $INIT_BASE/rc[0-6].d/[SK][0-9][0-9]ppr

	# Install the new links:
	for f in $INIT_LIST
	    do
	    echo "        $INIT_BASE/$f"
	    rm -f $INIT_BASE/$f
	    ln -s ../init.d/ppr $INIT_BASE/$f
	    done

	# Look for old PPR startup code in rc.local which may be
	# left from previous versions of PPR which didn't always
	# install System V init scripts when it was possible.
	if [ -n "$RC_LOCAL" ]
	    then
	    if grep '[Ss]tart [Pp][Pp][Rr]' $RC_LOCAL >/dev/null
		then
		echo
		echo "There are currently lines in $RC_LOCAL which start"
		echo "PPR.  You should remove these so that PPR does not get"
		echo "started twice."
		echo
		echo "Please press RETURN to continue."
		read x
		fi
	    fi

#
# Otherwise, try to find a Berkeley style rc.local file.
#
else
if [ -n "$RC_LOCAL" ]
    then
    if grep '[Ss]tart [Pp][Pp][Rr]' $RC_LOCAL >/dev/null
	then
	echo "Commands to start PPR already exist in $RC_LOCAL."
	echo "After this script is done, please make sure they are correct."
	echo "In particular, make sure that the program paths are correct."
	if grep "$CONFDIR/papsrv[^.]" $RC_LOCAL >/dev/null
	    then
	    echo "Also, change all references to \"$CONFDIR/papsrv\" to \"$CONFDIR/papsrv.conf\"."
	    fi
	echo
	echo "Please press RETURN to continue."
	read x

	else
	echo "Appending PPR startup commands to $RC_LOCAL"
	{
	echo
	echo "# ==== Start PPR ===="
	echo "# PPR always needs pprd.
	echo "if [ -x $HOMEDIR/bin/pprd ]"
	echo " then"
	echo " echo \"Starting PPRD\""
	echo " $HOMEDIR/bin/pprd"
	echo " fi"
	echo "# Start AppleTalk server only if config file is present.
	echo "if [ -x $HOMEDIR/bin/papsrv -a -r $CONFDIR/papsrv.conf ]"
	echo " then"
	echo " echo \"Starting PAPSRV\""
	echo " $HOMEDIR/bin/papsrv"
	echo " fi"
	echo "# Uncomment this line to run olprsrv in standalone mode."
	echo "#${HOMEDIR}/bin/olprsrv -s printer"
	echo "# Uncomment this line to run the new lprsrv in standalone mode."
	echo "#${HOMEDIR}/bin/lprsrv -s printer"
	echo "# ==== End of PPR Startup Code ===="
	} >>$RC_LOCAL

	fi

#
# No identifiable init script system.
#
    else
	clear
	echo "Since your system doesn't seem to use a System V style"
	echo "init nor do you have have an rc.local file, you must find"
	echo "your own means to start PPR when the system boots."
	echo "You must arrange for root to run $HOMEDIR/bin/pprd"
	echo "and possibly $HOMEDIR/bin/papsrv."
	echo
	echo "Please press RETURN to continue."
	read x
    fi

fi

echo "Done."
echo

exit 0

