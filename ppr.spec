#
# mouse:~ppr/src/ppr.spec
# Last modified 5 August 2003.
#

#
# This spec file hasn't been heavily tested.  I am sure it contains
# a few mistakes.  Please point them out.
#

#
# Building RPM packages as root is not recomended and is uncessary, at least
# in the case of PPR.  Instead, it is suggested that you run these commands 
# in your home directory to set up an RPM build space for yourself.
# 
# for d in BUILD SOURCES SPECS SRPMS RPMS/i386 RPMS/i486 RPMS/i586 RPMS/k6 RPMS/athlon RPMS/noarch
#	do
#	mkdir -p rpmbuild/$d
#	done
# echo "%_topdir $HOME/rpmbuild" >>.rpmmacros
# 
# You should then copy the PPR source archive into rpmbuild/SOURCES/ and
# run the following command:
# 
# rpmbuild -ba ppr.spec
#
# The source package will appear in rpmbuild/SOURCES and the binary package
# will appear in rpmbuild/RPMS/ in the directory for the appropriate
# architecture.
#

Summary: A spooler for PostScript printers
Name: ppr
Version: 1.52
Release: 1
Copyright: BSD
Group: System Environment/Daemons
Source: http://ppr.trincoll.edu/pub/ppr/ppr-%{version}.tar.gz
URL: http://ppr.trincoll.edu/
Vendor: Trinity College, Hartford, Connecticut, U.S.A.
Packager: Trinity College, Hartford, Connecticut, U.S.A.
BuildRoot: /var/tmp/%{name}-buildroot

#
# The as of 2 August 2003, Mandrake Cooker dependency scripts fill the 
# requires and provides list with very strange stuff, including an error
# message.  Among other things, the thing can't deal with script interpreters
# which are in the package itself.  It also generates dependencies for PPR's 
# *.pl files. 
#
# Selecting the program rpmdeps cuts down on the outright errors.  The grep
# commands filters out the really bad stuff.  No, PPR doesn't not depend on 
# Perl even though it can use it.
# 
AutoReq: yes
%define __find_requires       sh -c '/usr/lib/rpm/rpmdeps --requires $* | grep -v perl | grep -v tclsh | grep -v wish'

#
# Turn auto provides generation off completely since it doesn't find anything 
# useful to other packages, just a list of PPR's internal Perl modules.
#
AutoProv: no

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
#
# Note: Configure will take the CFLAGS from $RPM_OPT_FLAGS in preference
# to what it would have picked.
#============================================================================
%build
PERL=/usr/bin/perl GUNZIP=/usr/bin/gunzip \
	./Configure --prefix=/usr --user-ppr=ppr --with-gdbm --with-gettext --without-tdb
make

#============================================================================
# This does a dummy install of the program into a directory identified
# by $RPM_BUILD_ROOT.  This clean directory is used to create the cpio
# portion of the binary .rpm file.
#
# In case you are wondering, the PPR makefiles and the install scripts in 
# makeprogs/ automatically adjust their behavior when they see that
# $RPM_BUILD_ROOT is defined.
#============================================================================
%install
rm -rf $RPM_BUILD_ROOT
mkdir $RPM_BUILD_ROOT
touch root.sh
RPM_BUILD_ROOT=$RPM_BUILD_ROOT make install

#============================================================================
# This tells RPM what files go in the cpio section of the binary .rpm file.
#============================================================================
%files -f z_install_begin/installed_files_list
%docdir /usr/share/ppr/man
%docdir /usr/share/ppr/www/docs

#============================================================================
# This removes the build directory when starting a new build or after the
# RPM package has been built.
#============================================================================
%clean
rm -rf $RPM_BUILD_ROOT

#============================================================================
# This is run before unpacking the cpio archive from the binary .rpm file.
# This is similiar to what "make install" in z_install_begin/ does.
#
# It is annoying that the RPM documentation doesn't discuss the issue of
# creating users and groups.  It would be nice to specify the ID numbers,
# but we dasn't since something else might have them already.  We are
# reduced to doing a mass chown in the %post section.  Will this cause
# "rpm --verify" to report that PPR is corrupt?
#============================================================================
%pre

# Create the PPR users and groups.
/usr/sbin/groupadd ppr
/usr/sbin/useradd -M -d /usr/lib/ppr -c "PPR Spooling System" -g ppr -G lp ppr
/usr/sbin/useradd -M -d /usr/lib/ppr -c "PPR Spooling System" -g ppr pprwww

#============================================================================
# This is run after unpacking the cpio archive from the binary .rpm file.
# This is similiar to what make install in z_install_end/ does.
#============================================================================
%post

# Any files in the PPR directories which aren't owned by root must be
# supposed to be owned by the user ppr.  Make it so.
find /usr/lib/ppr /usr/share/ppr /var/spool/ppr /etc/ppr \
	-not -user 0 -not -group 0 -print0 \
    | xargs -0 chown ppr:ppr

# Initialize the binary media database.
touch /etc/ppr/media.db
chown ppr:ppr /etc/ppr/media.db
chmod 644 /etc/ppr/media.db
/usr/lib/ppr/bin/ppad media import /etc/ppr/media.sample >/dev/null

# Index fonts, PPD files, etc.
/usr/lib/ppr/bin/ppr-index >/dev/null

# Install crontab.
/usr/bin/crontab -u ppr - <<END
3 10,16 * * 1-5 /usr/lib/ppr/bin/ppad remind
5 4 * * * /usr/lib/ppr/lib/cron_daily
17 * * * * /usr/lib/ppr/lib/cron_hourly
END

# Install Inetd config.  (Xinetd is taken care of.)
# missing

# Setup init scripts to start PPR daemons at boot.
/sbin/chkconfig --add ppr

# Start PPR daemons.
/etc/rc.d/init.d/ppr start

#============================================================================
# This is run before uninstalling.
#============================================================================
%preun

# Stop the PPR daemons.
/etc/rc.d/init.d/ppr stop

# Remove init script links.
/sbin/chkconfig --del ppr

# Remove the crontab.
/usr/bin/crontab -u ppr -r

# Remove the UPRINT symbolic links and put the native spooler programs back.
/usr/lib/ppr/bin/uprint-newconf --remove >/dev/null

# If PPR has lines in /etc/inetd.conf, remove them and tell Inetd to reload
# its configuration file.
if [ -f /etc/inetd.conf ]
    then
    if grep /usr/lib/ppr/lib/ /etc/inetd.conf >/dev/null
	then
	rm -f /etc/inetd.conf~
	mv /etc/inetd.conf /etc/inetd.conf~
	grep -v '/usr/lib/ppr/lib/' /etc/inetd.conf~ >/etc/inetd.conf
	killall -HUP inetd 2>/dev/null
	fi
    fi

# Tell Xinetd to reload its configuration files.
if [ -d /etc/xinetd.d ]
    then
    killall -HUP xinetd 2>/dev/null
    fi

# Remove almost everything PPR ever generated.  This includes the indexes.
/usr/lib/ppr/bin/ppr-clean --all-removable >/dev/null

#============================================================================
# This is run after uninstalling.
#============================================================================
%postun

# Remove the PPR users and groups.
/usr/sbin/userdel ppr
/usr/sbin/userdel pprwww
/usr/sbin/groupdel ppr

# end of file
