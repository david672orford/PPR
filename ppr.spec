#
# mouse:~ppr/src/ppr.spec
# Last modified 21 February 2003.
#
# This spec file hasn't been heavily tested.  I am sure it contains
# a few mistakes.  Please point them out.
#
# In order to use this file, move the PPR source archive to
# /usr/src/RPM/SOURCES and run "rpm -ba ppr-X.XX.spec".  It is possible to
# build the thing elsewhere, such as in one's home dirctory, but I don't know
# how.
#

Summary: A spooler for PostScript printers
Name: ppr
Version: 1.51
Release: 1
Copyright: BSD
Group: System Environment/Daemons
Source: http://ppr.trincoll.edu/pub/ppr/ppr-%{version}.tar.gz
URL: http://ppr.trincoll.edu/
Vendor: Trinity College, Hartford, Connecticut, U.S.A.
Packager: Trinity College, Hartford, Connecticut, U.S.A.
BuildRoot: /var/tmp/%{name}-buildroot

%description
PPR is a print spooler for PostScript printers.  It can print to parallel,
serial, AppleTalk, LPR/LPD, SocketAPI, and SMB printers.  It works well
with Ghostscript, Netatalk, CAP60, and Samba.

#============================================================================
# This unpacks the source.
#============================================================================
%prep
%setup -q

#============================================================================
# This configures and builds the source.  The current directory is already
# the root of the source code.
#============================================================================
%build

# Use the new, experimental configure script since it can mostly run
# non-interactively.
# Note: We have to work %_target_cpu in here somewhere.  Right now
# it configures for the CPU it runs on.
./Configure_new --prefix=/usr --user-ppr=ppr --with-gdbm --with-gettext

make

#============================================================================
# This does a dummy install of the program into a directory identified
# by $RPM_BUILD_ROOT.  This clean directory is used to create the cpio
# portion of the binary .rpm file.
#============================================================================
%install
rm -rf $RPM_BUILD_ROOT
mkdir $RPM_BUILD_ROOT
RPM_BUILD_ROOT=$RPM_BUILD_ROOT make install

#============================================================================
# This tells RPM what files go in the cpio section of the binary .rpm file.
#============================================================================
%files -f z_install_end/installed_files_list
%docdir /usr/share/ppr/man
%docdir /usr/share/ppr/www/docs
#%config /etc/ppr/media.db

#============================================================================
# This removes the build directory after the install.
#============================================================================
%clean
rm -rf $RPM_BUILD_ROOT

#============================================================================
# This is run before unpacking the cpio archive from the binary .rpm file.
#============================================================================
%pre

#============================================================================
# This is run after unpacking the cpio archive from the binary .rpm file.
#============================================================================
%post
/usr/lib/ppr/fixup/fixup >/dev/null
/etc/rc.d/init.d/ppr start

#============================================================================
# This is run before uninstalling.
#
# We have to remove a lot of stuff that PPR creates as it runs or that
# is created by fixup.  There is probably a better solution, but we will
# do it this way until the have fully described the problem by means of
# this code.
#============================================================================
%preun

# Stop the PPR daemons.
/etc/rc.d/init.d/ppr stop

# Remove init scripts.
/sbin/chkconfig --del ppr
rm /etc/rc.d/init.d/ppr

# Remove resource cache.
rm -f /var/spool/ppr/cache/*/*

# Remove boring log files.
for l in pprd pprd.old pprdrv papsrv papd lprsrv ppr-indexfonts ppr-clean ppr-httpd uprint
    do
    rm -f /var/spool/ppr/logs/$l
    done

# Remove shell script filters.
for t in pr ditroff troff dvi tex texinfo pdf html jpeg gif bmp pnm xbm xpm xwd tiff png plot fig
    do
    rm -f /usr/lib/ppr/filters/filter_$t
    done

# Remove Samba spool files.
rm -f /var/spool/ppr/sambaspool/*

# Remove printer status stuff.
rm -f /var/spool/ppr/printers/addr_cache/*
rm -f /var/spool/ppr/printers/alerts/*
rm -f /var/spool/ppr/printers/status/*

# Remove client spooling stuff.
rm -f /var/spool/ppr/pprclipr/*

# Remove DVIPS config files.
rm -f /var/spool/ppr/dvips/*

# Remove print jobs.
rm -f /var/spool/ppr/queue/*
rm -f /var/spool/ppr/jobs/*

# Remove the font index.
rm -f /var/spool/ppr/fontindex.db

# Remove pprd's FIFO.
rm -f /var/spool/ppr/PIPE

# Remove the communication files.
rm -f /var/spool/ppr/run/state_update
rm -f /var/spool/ppr/run/state_update_pprdrv

# Remove links in /usr/bin.
rm -f /usr/bin/ppr \
	/usr/bin/ppop \
	/usr/bin/ppad \
	/usr/bin/ppuser \
	/usr/bin/ppdoc \
	/usr/bin/ppr-xgrant \
	/usr/bin/ppr-config \
	/usr/bin/ppr-web \
	/usr/bin/ppr-passwd

# Remove the UPRINT symbolic links and put the native spooler programs back.
/usr/lib/ppr/bin/uprint-newconf --remove

# Remove ACL files that are still empty.
for l in pprprox.allow ppop.allow ppad.allow ppuser.allow passwd.allow
    do
    if [ -f /etc/ppr/acl/$l -a ! -s /etc/ppr/acl/$l ]
	then
	rm -f /etc/ppr/acl/$l
	fi
    done

# Remove configuration files that are identical to the samples.
for f in ppr.conf uprint.conf uprint-remote.conf lprsrv.conf
    do
    if diff /etc/ppr/$f /etc/ppr/$f.sample >/dev/null
    	then
    	rm -f /etc/ppr/$f
    	fi
    done

# Remove the generated sample configuration file.
rm -f /etc/ppr/ppr.conf.sample

# Remove the PPR entries from Inetd's config file.
rm -f /etc/inetd.conf~
mv /etc/inetd.conf /etc/inetd.conf~
grep -v '/usr/lib/ppr/' </etc/inetd.conf~ >/etc/inetd.conf
killall -HUP inetd

# Do the same for Xinetd.
if [ -f /etc/xinetd.c/ppr ]
    then
    rm -f /etc/xinetd.d/ppr
    killall -HUP xinetd
    fi

# Remove the crontab.
/usr/bin/crontab -u ppr -r

# Remove the PPR users and groups.
/usr/sbin/userdel ppr
/usr/sbin/userdel pprwww
/usr/sbin/groupdel ppr

#============================================================================
# This is run after uninstalling.
#============================================================================
%postun

# end of file
