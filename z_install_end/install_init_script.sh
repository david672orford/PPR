#! /bin/sh
#
# mouse:~ppr/src/z_install_end/install_init_script.sh
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
# Last modified 24 March 2003.
#

. ../makeprogs/paths.sh

#========================================================================
# Part 1, figure out what we have
#========================================================================

echo "  Studying Init script system..."

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
	    INIT_LIST="rc0.d/K40ppr rc1.d/K40ppr rc2.d/S80ppr rc3.d/S80ppr rc4.d/S80ppr rc5.d/S80ppr rc6.d/K40ppr"
	    ;;

	# Several systems
	"/etc" )
	    # Debian Linux
	    if [ -x /usr/bin/dpkg -a -x /usr/sbin/update-rc.d ]
	    then
	    # Needs checking:
	    INIT_LIST="rc0.d/K20ppr rc2.d/S80ppr rc3.d/S80ppr rc4.d/S80ppr rc5.d/S80ppr rc6.d/K20ppr"
	    else

	    # Solaris 2.x, Generic System V
	    INIT_LIST="rc0.d/K20ppr rc2.d/S80ppr"
	    fi
	    ;;

	# OSF/1 3.2
    	"/sbin" )
	    # Needs checking:
	    INIT_LIST="rc0.d/K00ppr rc2.d/K00ppr rc3.d/S65ppr"
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

#
# Look for MacOS StartupItems
#
StartupItems=""
for i in /System/Library/StartupItems 
    do 
    if [ -d $i ]
	then
	echo "    MacOS StartupItems found at \"$i\"."
    	StartupItems=$i
	fi
    done

#========================================================================
# Part 2, install our code
#========================================================================

#
# If we have a System V style init with System V style init scripts, install
# a startup script for PPR and make links in the directories for the
# appropriate run levels.
#
if [ -n "$INIT_BASE" ]
then
	echo "  Installing the System V style start and stop scripts..."

	# Install the principal script if it isn't already installed.
	../makeprogs/installprogs.sh root root 755 $INIT_BASE/init.d ppr
	if [ $? -ne 0 ]
	    then
	    echo "===================================================="
	    echo "Please run again as root to update init script."
	    echo "===================================================="
	    exit 1
	    fi

	# Remove any old links and replace them with new ones.
	existing=`echo $INIT_BASE/rc[0-6].d/[SK][0-9][0-9]ppr`
	temp=""
	for l in $INIT_LIST
	    do
	    temp="$temp $INIT_BASE/$l"
	    done
	#echo "$temp"
	#echo "$existing"
	if [ "$temp" = " $existing" ]
	    then
	    echo "    Links are already correct."
	    else
	    if [ -x /sbin/chkconfig ]
		then
		/sbin/chkconfig --add ppr
		else
		for l in $existing
		    do
		    if [ -f $l ]
			then
			rm $l
			if [ $? -ne 0 ]
			    then
			    echo "Please run again as root to update init links."
			    exit 1
			    fi
			fi
		    done
		for f in $INIT_LIST
		    do
		    echo "    $INIT_BASE/$f"
		    rm -f $INIT_BASE/$f
		    ln -s ../init.d/ppr $INIT_BASE/$f
		    done
	        fi
	    fi

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
# Otherwise, do the MacOS X thing.
#
else
if [ -n "$StartupItems" ]
    then
    echo "  Installing MacOS X startup script..."
    if [ -d $StartupItems/PPR ]
	then
	echo "    Directory $StartupItems/PPR already exists, good."
	else
	echo "    Creating directory $StartupItems/PPR..."
	mkdir $StartupItems/PPR
	fi
    if diff ppr $StartupItems/PPR/PPR >/dev/null 2>&1
	then
	echo "    Startup script already installed, good."
	else
	echo "    Installing startup script..."
	cp ppr $StartupItems/PPR/PPR || exit 1
	fi
    if [ -f $StartupItems/PPR/StartupParameters.plist ]
	then
	echo "    $StartupItems/PPR/StartupParameters.plist already exists, good."
	else
	echo "    Creating $StartupItems/PPR/StartupParameters.plist..."
	cat - >$StartupItems/PPR/StartupParameters.plist <<"EndOfStartupParameters"
{
  Description     = "PPR Print Spooler";
  Provides        = ("PPR Printing");
  Requires        = ("Resolver");
  Uses            = ("Network Time");
  OrderPreference = "Late";
}
EndOfStartupParameters
	fi

#
# Otherwise, try to find a Berkeley style rc.local file.
#
else
if [ -n "$RC_LOCAL" ]
    then
    if grep '[Ss]tart [Pp][Pp][Rr]' $RC_LOCAL >/dev/null
	then
	echo
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
	echo

	else
	echo "  Appending PPR startup commands to $RC_LOCAL"
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
	echo
	echo "Since your system doesn't seem to use a System V style Init nor do you have"
	echo "have an rc.local file, you must find your own means to start PPR when the"
	echo "system boots.  Specifically, you must arrange for root or $USER_PPR to run"
	echo "$HOMEDIR/bin/pprd and possibly $HOMEDIR/bin/papsrv."
	echo
	echo "Please press RETURN to continue."
	read x
	echo
    fi

fi
fi

exit 0
