#
# mouse:~ppr/src/z_install_end/fixup_obsolete.sh
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
# Last modified 16 April 2004.
#

. ../makeprogs/paths.sh

#===========================================================================
# Remove files which are no longer used in the current version or PPR.
#===========================================================================

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

# 1.40
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

# PPR 1.41
outdated $HOMEDIR/lib/paths.sh				# never was used
outdated $HOMEDIR/lib/PrintDesk/PPRcontrol.pm
outdated $SHAREDIR/www/logs.html
outdated $SHAREDIR/www/docbrowse.html
outdated $SHAREDIR/www/show_logs.html
outdated $SHAREDIR/www/test-audio-commentary.html
outdated $SHAREDIR/www/test-audio-responder.html
outdated $SHAREDIR/www/test.html
outdated $SHAREDIR/www/docs/rfc/index.html

# PPR 1.41
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
$SHAREDIR and $HOMEDIR.	 Only a few symbolic links have been left
here in case $CONFDIR/ppr.conf, /etc/inetd.conf, /etc/passwd, or
Samba's smb.conf refer to them.	 Once you have corrected all three
of those files, you may remove this directory.

The former contents of this directory are preserved as /usr/ppr_old.
EndOfQuote
	if [ ! -d $SHAREDIR/speach -a -d /usr/ppr_old/speach ]
	then
	echo "Moving \"/usr/ppr_old/speach\" to \"$SHAREDIR/speach\"." || exit 1
	fi
	fi

# PPR 1.42
outdated $HOMEDIR/lib/ppr-commentary-httpd
outdated_dir $SHAREDIR/www/docs/rfc
outdated $SHAREDIR/www/html/docbrowse.html

# PPR 1.50
outdated $HOMEDIR/lib/login_ppr.sh
outdated $HOMEDIR/lib/login_ppr.csh
#outdated $HOMEDIR/lib/pprpopupd			# uncomment for 1.90
outdated $HOMEDIR/cgi-bin/df_img.cgi
outdated "$SHAREDIR/PPDFiles/HP DeskJet 870C Ghostscript"
outdated "$SHAREDIR/PPDFiles/HP LaserJet 4050 N"	# is bad
#outdated "$SHAREDIR/PPDFiles/HP C LaserJet 4500-PS"
outdated "$SHAREDIR/PPDFiles/HP LaserJet 4050 N"	# is bad
outdated "$SHAREDIR/PPDFiles/Dot Matrix 24 pin Ghostscript"
outdated $HOMEDIR/bin/ppr-web-control			# renamed
outdated $HOMEDIR/bin/papsrv-kill			# replaced by papsrv --stop

# PPR 1.51
outdated $HOMEDIR/fixup/fixup_login
outdated $HOMEDIR/fixup/login_ppr.sh
outdated $HOMEDIR/fixup/login_ppr.csh
outdated $HOMEDIR/fixup/remove_ppr
outdated $HOMEDIR/fixup/fixup_conf
outdated $HOMEDIR/fixup/fixup_samples
outdated $HOMEDIR/fixup/fixup_filters
outdated $HOMEDIR/fixup/fixup_atalk
outdated $HOMEDIR/fixup/fixup_media
outdated $HOMEDIR/fixup/fixup
outdated $HOMEDIR/fixup/fixup_accounts
outdated $HOMEDIR/fixup/fixup_cron
outdated $HOMEDIR/fixup/fixup_inetd
outdated $HOMEDIR/fixup/fixup_init
outdated $HOMEDIR/fixup/fixup_links
outdated $HOMEDIR/fixup/fixup_lmx
outdated $HOMEDIR/fixup/fixup_obsolete
outdated $HOMEDIR/fixup/fixup_perms
outdated $HOMEDIR/fixup/customs.ppr
outdated $HOMEDIR/fixup/init_ppr
outdated $HOMEDIR/lib/getpwnam
outdated $HOMEDIR/lib/getgrnam
outdated $HOMEDIR/bin/ppr-indexfonts
outdated $HOMEDIR/bin/ppr-indexppds
outdated $VAR_SPOOL_PPR/logs/ppr-indexfonts
outdated $VAR_SPOOL_PPR/logs/ppr-indexppds

# PPR 1.53
outdated $HOMEDIR/cgi-bin/show_queues_nojs.cgi
outdated_dir $SHAREDIR/locale/ru
outdated_dir $SHAREDIR/locale/fr
outdated $HOMEDIR/lib/olprsrv
outdated $HOMEDIR/lib/PrintDesk/ATChooser.pm
outdated $HOMEDIR/bin/ppr-chooser
outdated $HOMEDIR/lib/getzones

exit 0
