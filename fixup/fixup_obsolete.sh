#
# mouse:~ppr/src/fixup/fixup_obsolete.sh
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
# Last modified 29 August 2002.
#

HOMEDIR="?"
SHAREDIR="?"
CONFDIR="?"
VAR_SPOOL_PPR="?"
BINDIR="?"

#===========================================================================
# Remove files which are no longer used in the current version or PPR.
#===========================================================================

echo "Removing obsolete files..."

outdated ()
  {
  if [ -f "$1" ]
    then
    echo "$1"
    rm -f "$1" || exit 1
    if [ "$2" != "" ]
      then
      echo "    (Linking to $2 for backward compatibility.)"
      ln -s "$2" "$1" || exit 1
      fi
  fi
  }

outdated_dir ()
  {
  if [ -d "$1" ]
    then
    echo "$1"
    rm -rf "$1" || exit 1
  fi
  }

outdated $VAR_SPOOL_PPR/pprd.lock
outdated $VAR_SPOOL_PPR/cache/file
outdated $BINDIR/xppr $BINDIR/xpprstat
outdated $HOMEDIR/interfaces/gs
outdated $HOMEDIR/bin/fake_lp
outdated $HOMEDIR/bin/fake_lpr
outdated $HOMEDIR/bin/fake_lpq
outdated $HOMEDIR/bin/fake_lprm
outdated $CONFDIR/lw_errors.conf
outdated $CONFDIR/lw_status.conf
outdated $CONFDIR/lw_messages.conf
outdated $HOMEDIR/lib/nlprsrv
outdated $HOMEDIR/bin/uprint_lp
outdated $HOMEDIR/bin/uprint_lpr
outdated $HOMEDIR/bin/uprint_lpq
outdated $HOMEDIR/bin/uprint_lprm
outdated $HOMEDIR/bin/xppr
outdated $HOMEDIR/bin/xpprstat
outdated $HOMEDIR/interfaces/regtest
outdated $VAR_SPOOL_PPR/cache/procset/AdobePS_Win_Driver_L2-4.2-0
outdated $HOMEDIR/bin/samba_submitter
outdated $HOMEDIR/bin/xerox_ticket_submitter
outdated $CONFDIR/printcap
outdated_dir $HOMEDIR/htmldocs
outdated $HOMEDIR/bin/pprsync
outdated $CONFDIR/lw-messages.conf.sample
outdated $CONFDIR/lw-messages.conf
outdated $CONFDIR/ttfonts			# 1st
outdated $CONFDIR/ttfonts.db			# 2nd
outdated $VAR_SPOOL_PPR/ttfonts.db		# 3rd?
outdated $HOMEDIR/bin/xfm-ppr $HOMEDIR/bin/ppr-xfm
outdated $HOMEDIR/bin/xpprgrant $HOMEDIR/bin/ppr-xgrant
outdated $HOMEDIR/bin/pprclean
outdated $HOMEDIR/bin/papsrv_kill $HOMEDIR/bin/papsrv-kill
outdated $HOMEDIR/lib/xerox_ticket_submitter
outdated $HOMEDIR/bin/indexttf
outdated_dir $SHAREDIR/globalconf

# These have been renamed.
for f in \
	ASCIIEncoding \
	CP1250Encoding \
	CP437Encoding \
	ISOLatin2Encoding \
	ISOLatin3Encoding \
	ISOLatin4Encoding \
	ISOLatin5Encoding \
	KOI8Encoding \
	MacintoshEncoding
    do
    outdated $SHAREDIR/cache/encoding/$f
    done

# These are obsolete.
for f in \
	TrinColl-PPR-dmm-nup-3-3 \
	TrinColl-PPR-dmm-nup-3-4 \
	TrinColl-PPR-dmm-nup-3-5 \
	TrinColl-PPR-ReEncode-1-0
    do
    outdated $SHAREDIR/cache/procset/$f
    done

outdated_dir $VAR_SPOOL_PPR/addr_cache		# moved
outdated_dir $VAR_SPOOL_PPR/alerts		# moved
outdated $VAR_SPOOL_PPR/nextid			# moved
outdated $VAR_SPOOL_PPR/lpr_nextid		# older than the one below
outdated $VAR_SPOOL_PPR/lpr_previd		# newer but still obsolete
outdated $VAR_SPOOL_PPR/pprd.pid		# moved
outdated $VAR_SPOOL_PPR/papsrv.pid		# moved
outdated $VAR_SPOOL_PPR/lprsrv.pid		# moved
outdated $VAR_SPOOL_PPR/olprsrv.pid		# moved
outdated $VAR_SPOOL_PPR/state_update		# moved
outdated $VAR_SPOOL_PPR/state_update_pprdrv	# moved
outdated $HOMEDIR/lib/Printdesk/printer2.xbm

if [ -d "$VAR_SPOOL_PPR/win95drv" ]
    then
    mv $VAR_SPOOL_PPR/win95drv/* $VAR_SPOOL_PPR/drivers/win95
    outdated_dir "$VAR_SPOOL_PPR/win95drv"
    fi

outdated $HOMEDIR/lib/cgi_conf.pl
outdated $HOMEDIR/lib/play_local.pl
outdated $HOMEDIR/lib/pprwatch
outdated $HOMEDIR/lib/paths.pl
outdated $HOMEDIR/bin/ttf2ps
outdated $HOMEDIR/bin/printdesk
outdated $HOMEDIR/bin/atchooser
outdated $HOMEDIR/cgi-bin/job_properties.cgi
outdated $VAR_SPOOL_PPR/drivers/win95/win95cp.bat	# changed to upper case
outdated "$SHAREDIR/cache/procset/ApplDict md-71-0 "	# created accidentally
outdated $CONFDIR/smb-protos.conf $HOMEDIR/lib/smb-protos.conf
outdated $CONFDIR/smb-protos.conf.sample
outdated_dir $HOMEDIR/install
outdated $HOMEDIR/lib/paths.sh				# never was used
outdated $HOMEDIR/lib/PrintDesk/PPRcontrol.pm
outdated $SHAREDIR/www/logs.html
outdated $SHAREDIR/www/docbrowse.html
outdated $SHAREDIR/www/show_logs.html
outdated $SHAREDIR/www/test-audio-commentary.html
outdated $SHAREDIR/www/test-audio-responder.html
outdated $SHAREDIR/www/test.html
outdated $SHAREDIR/www/docs/rfc/index.html
outdated $HOMEDIR/lib/ppr-commentary-httpd
outdated_dir $SHAREDIR/www/docs/rfc
outdated $SHAREDIR/www/html/docbrowse.html
outdated $HOMEDIR/lib/login_ppr.sh
outdated $HOMEDIR/lib/login_ppr.csh
#outdated $HOMEDIR/lib/pprpopupd			# uncomment for 1.90
outdated $HOMEDIR/cgi-bin/df_img.cgi

outdated "$SHAREDIR/PPDFiles/HP DeskJet 870C Ghostscript"
outdated "$SHAREDIR/PPDFiles/HP LaserJet 4050 N"	# is bad
#outdated "$SHAREDIR/PPDFiles/HP C LaserJet 4500-PS"
outdated "$SHAREDIR/PPDFiles/HP LaserJet 4050 N"	# is bad
outdated "$SHAREDIR/PPDFiles/Dot Matrix 24 pin Ghostscript"

echo "Done."
echo

#===========================================================================
# Remove obsolete man pages.
#
# Before PPR 1.40a4 these man pages were installed in the tree rooted at
# /usr/local/man.  Now PPR keeps them in its own man directory.  If the
# old ones were not removed someone might get them instead of the new
# ones if he hadn't put the new PPR man directory in his MANPATH.
#
#===========================================================================

echo "Removing obsolete man pages..."

MANDIR=/usr/local/man

if [ -d $MANDIR ]
    then
    cd $MANDIR/man1
    rm -f UPRINT.1 uprint-*.1 xfm-ppr.1 xppr.1 xpprgrant.1

    cd $MANDIR/man8
    rm -f indexttf.8 lprsrv.8 olprsrv.8 ppr2samba.8 samba_submitter.8 ppuser.8 pprd.8 papsrv.8

    cd $MANDIR/man5
    rm -f lw-messages.conf.5 mfmodes.conf.5 fontsub.conf.5 uprint-remote.conf.5
    fi

echo "Done."
echo

#===========================================================================
# Rename files whose names have changed in the current version:
#===========================================================================

renamed ()
{
if [ -f $1 ]
	then echo "mv $1 $2"
	mv $1 $2 || exit 1
	fi
}

echo "Renaming files whose names have changed..."
renamed $CONFDIR/charge_users $CONFDIR/charge_users.db
renamed $CONFDIR/newprn $CONFDIR/newprn.conf
renamed $CONFDIR/papsrv $CONFDIR/papsrv.conf
renamed $CONFDIR/papsrv_default_zone $CONFDIR/papsrv_default_zone.conf
echo "Done."
echo

echo "Renaming config files which won't work anymore..."
renamed $CONFDIR/fontsub $CONFDIR/fontsub-pre-1.30.conf
renamed $CONFDIR/media $CONFDIR/media-pre-1.30.db
renamed $CONFDIR/mfmodes $CONFDIR/mfmodes-pre-1.30.conf
renamed $CONFDIR/fontsub $CONFDIR/fontsub-pre-1.30.conf
renamed $CONFDIR/mfmodes $CONFDIR/mfmodes-pre-1.30.conf
renamed $CONFDIR/fontsub.conf $CONFDIR/fontsub-pre-1.40.conf
renamed $CONFDIR/mfmodes.conf $CONFDIR/mfmodes-pre-1.40.conf
echo "Done."
echo

echo "Moving filters to new location..."
for f in $HOMEDIR/lib/filter_*
    do
    if [ -f $f ]
	then
	base=`basename $f`
	echo "Moving \"$f\" to \"$HOMEDIR/filters/$base\"."
	mv $f $HOMEDIR/filters/$base || exit 1
	fi
    done
echo "Done."
echo

echo "Checking of symbolic links from old install directories are needed..."
if [ $HOMEDIR != "/usr/ppr" -a -d /usr/ppr -a ! -d /usr/ppr_old ]
    then
    echo "Moving \"/usr/ppr\" to \"/usr/ppr_old\"."
    mv /usr/ppr /usr/ppr_old || exit 1
    echo "Creating a new /usr/ppr for compatibility links."
    mkdir /usr/ppr || exit 1
    ln -s $SHAREDIR/fonts /usr/ppr/fonts || exit 1
    ln -s $HOMEDIR/lib /usr/ppr/lib || exit 1
    ln -s $HOMEDIR/bin /usr/ppr/bin || exit 1
    cat >/usr/ppr/README.txt <<EndOfQuote
This directory was once part of PPR.  Its contents have been moved to
$SHAREDIR and $HOMEDIR.  Only a few symbolic links have been left
here in case $CONFDIR/ppr.conf, /etc/inetd.conf, /etc/passwd, or
Samba's smb.conf refer to them.  Once you have corrected all three
of those files, you may remove this directory.

The former contents of this directory are preserved as /usr/ppr_old.
EndOfQuote
    if [ ! -d $SHAREDIR/speach -a -d /usr/ppr_old/speach ]
	then
	echo "Moving \"/usr/ppr_old/speach\" to \"$SHAREDIR/speach\"." || exit 1
	fi
    echo "Done."
    else
    echo "Not needed."
    fi
echo

exit 0

