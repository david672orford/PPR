#
# mouse:~ppr/src/ppr.spec
# Last modified 5 April 2003.
#

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
with Ghostscript, Netatalk, CAP60, and Samba.  It has a web interface.

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
./Configure --prefix=/usr --user-ppr=ppr --with-gdbm --with-gettext --with-tdb

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
%files -f z_install_begin/installed_files_list
%docdir /usr/share/ppr/man
%docdir /usr/share/ppr/www/docs

#============================================================================
# This removes the build directory after the install.
#============================================================================
%clean
rm -rf $RPM_BUILD_ROOT

#============================================================================
# This is run before unpacking the cpio archive from the binary .rpm file.
#============================================================================
%pre

# Create the PPR users and groups.
/usr/sbin/groupadd ppr
/usr/sbin/useradd -M -d /usr/lib/ppr -c "PPR Spooling System -g ppr -G lp ppr
/usr/sbin/useradd -M -d /usr/lib/ppr -c "PPR Spooling System -g ppr pprwww

#============================================================================
# This is run after unpacking the cpio archive from the binary .rpm file.
#============================================================================
%post

find /usr/lib/ppr /usr/share/ppr /var/spool/ppr /etc/ppr -not -user 0 -not -group 0 | xargs chown ppr:ppr

/usr/lib/ppr/bin/ppad media import /etc/ppr/media.sample >/dev/null

/usr/lib/ppr/bin/ppr-index >/dev/null

/sbin/chkconfig --add ppr

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

# Remove init script links.
/sbin/chkconfig --del ppr

# Remove the crontab.
/usr/bin/crontab -u ppr -r

# Remove the UPRINT symbolic links and put the native spooler programs back.
/usr/lib/ppr/bin/uprint-newconf --remove

# Remove PPR from /etc/inetd.conf.
if [ -f /etc/inetd.conf ]
    then
    if grep /usr/lib/ppr/bin/ /etc/inetd.conf >/dev/null
	then
	rm -f /etc/inetd.conf~
	mv /etc/inetd.conf /etc/inetd.conf~
	grep -v '/usr/lib/ppr/bin/' /etc/inetd.conf~ >/etc/inetd.conf
	fi
    fi

# Remove almost everything PPR ever generated.
/usr/lib/ppr/bin/ppr-clean --all-removable

#============================================================================
# This is run after uninstalling.
#============================================================================
%postun

# Let Inetd pick up the new configuration.
killall -HUP inetd
killall -HUP xinetd

# Remove the PPR users and groups.
/usr/sbin/userdel ppr
/usr/sbin/userdel pprwww
/usr/sbin/groupdel ppr

# end of file
