#
# mouse:~ppr/src/ppr.spec
# Last modified: 16 May 2014
#

Name: ppr
Summary: A spooler for PostScript printers
Version: 2.00a1
Release: 1
License: BSD
URL: http://ppr.trincoll.edu/
Source: http://ppr.trincoll.edu/pub/ppr/ppr-%{version}.tar.gz
Packager: David Chappell <ppr-bugs@trincoll.edu>
BuildRoot: %{_tmppath}/%{name}-buildroot

# Mandrake
Distribution: Mandrake Linux
Vendor: MandrakeSoft
Group: System/Servers

# RedHat
#Distribution:
#Vendor:
#Group: System Environment/Daemons

#============================================================================
#
# Building RPM packages as root is not recomended and is uncessary, at least
# in the case of PPR.  Instead, it is suggested that you run these commands 
# in your home directory to set up an RPM build space for yourself.
# 
# mkdir rpmbuild/BUILD
# mkdir rpmbuild/SOURCES
# mkdir rpmbuild/SPECS
# mkdir rpmbuild/SRPMS
# mkdir rpmbuild/RPMS
# mkdir rpmbuild/RPMS/i386
# mkdir rpmbuild/RPMS/i586
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
#============================================================================

#
# This filters out spurious requirements.  PPR is designed to run without
# Perl.  The Tcl/Tk requirement is a result of pprpopup, a very optional
# part of PPR.  Lots of spurious completely imaginary Perl requirements 
# are generated too.
#
%define __find_requires sh -c '/usr/lib/rpm/rpmdeps --requires $* | grep -v perl | grep -v tclsh | grep -v wish'

#
# Turn auto provides generation off completely since it doesn't find anything 
# useful to other packages, just a list of PPR's internal Perl modules.
#
AutoProv: no

%description
PPR is a print spooler for PostScript printers.  It can print to parallel,
serial, AppleTalk, LPR/LPD, SocketAPI, and SMB printers.  It works well
with Ghostscript, Netatalk, CAP60, and Samba.  It has a web interface.

%changelog
* Tue Aug 5 2003 David Chappell <David.Chappell@trincoll.edu> 1.52-1
- for PPR 1.52 release

* Tue Dec 11 2004 David Chappell <David.Chappell@trincoll.edu> 1.53-1
- for PPR 1.53 release

* Tue May 16 2014 David Chappell <David.Chappell@trincoll.edu> 2.00a1-1
- for PPR 2.00a1 release

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
CFLAGS="$RPM_OPT_FLAGS" PERL=/usr/bin/perl GUNZIP=/usr/bin/gunzip \
	./Configure --prefix=/usr --user-ppr=ppr --with-gdbm --with-gettext --without-tdb
make

#============================================================================
# This does a dummy install of the program into a directory whose name is
# found in $RPM_BUILD_ROOT.  This clean directory is used to create the
# file archive portion of the binary .rpm file.
#
# In case you are wondering, the PPR makefiles and the install scripts in 
# makeprogs/ automatically adjust their behavior when they see that
# $RPM_BUILD_ROOT is defined.
#
# The file root.sh gives makeprogs/install*.sh a place to vent when they get
# angry that they can't set the file permissions they want.
#============================================================================
%install
rm -rf $RPM_BUILD_ROOT
mkdir $RPM_BUILD_ROOT
touch root.sh
RPM_BUILD_ROOT=$RPM_BUILD_ROOT make install

#============================================================================
# This tells RPM what files go in the cpio section of the binary .rpm file.
# The files list is generated by makeprogs/install*.sh.
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
# This is run before unpacking the file archive from the binary .rpm file.
# This is similiar to what "make install" in z_install_begin/ does.
#============================================================================
%pre

#echo "pre of %{name}-%{version}-%{release}: count=$1"

# If this is a first time install, and not an upgrade,
if [ $1 -lt 2 ]
	then
	#echo "  Is a new install."

	# Create the PPR users and groups.
	/usr/sbin/groupadd ppr
	/usr/sbin/useradd -M -d /usr/lib/ppr -c "PPR Spooling System" -g ppr -G lp ppr
	/usr/sbin/useradd -M -d /usr/lib/ppr -c "PPR Spooling System" -g ppr pprwww
	fi

true

#============================================================================
# This is run after unpacking the archive from the binary .rpm file.
# This is similiar to what make install in z_install_end/ does.
#============================================================================
%post

#echo "post of %{name}-%{version}-%{release}: count=$1"

# Sample empty files are not of value.
#rm -f /etc/ppr/acl/*.rpmnew

# These will just be copies of the .sample files.  Nix them.
#rm -f /etc/ppr/*.rpmnew

# Import standard media types into the media database.
/usr/lib/ppr/bin/ppad media import /etc/ppr/media.sample >/dev/null

# Generate or re-generate index of fonts, PPD files, etc.
/usr/lib/ppr/bin/ppr-index >/dev/null

# Install user ppr's current crontab.
/usr/bin/crontab -u ppr - <<END
3 10,16 * * 1-5 /usr/lib/ppr/bin/ppad remind
5 4 * * * /usr/lib/ppr/lib/cron_daily
17 * * * * /usr/lib/ppr/lib/cron_hourly
END

# If this isn't an upgrade,
if [ $1 -lt 2 ]
	then
	#echo "  Is a new install, not an upgrade."

	# Setup init scripts to start PPR daemons at boot.
	/sbin/chkconfig --add ppr

	# Tell Xinetd to pick up the new PPR services.  Yes, HUP is right.
	# See the xinetd(8) manpage if you don't believe me.
	killall -HUP xinetd
	fi

# If PPR is running, restart it.
command=`/etc/rc.d/init.d/ppr probe`
[ -n "$command" ] && /etc/rc.d/init.d/ppr $command

true

#============================================================================
# This is run before uninstalling.
#============================================================================
%preun

#echo "preun of %{name}-%{version}-%{release}: count=$1"

# If this is an actual removal and not an upgrade,
if [ $1 -lt 2 ]
	then
	#echo "  Is an actual removal, not an upgrade."

	# Stop the PPR daemons while the stop script is still available.
	/etc/rc.d/init.d/ppr stop >/dev/null

	# Remove init script links.
	/sbin/chkconfig --del ppr

	# Remove the UPRINT symbolic links and put the native spooler programs back.
	/usr/lib/ppr/bin/uprint-newconf --remove >/dev/null

	# Remove almost everything PPR ever generated.  This includes the indexes.
	/usr/lib/ppr/bin/ppr-clean --all-removable >/dev/null

	# Remove the crontab.
	/usr/bin/crontab -u ppr -r
	fi

true

#============================================================================
# This is run after uninstalling (and possibly after installing a newer
# version).
#============================================================================
%postun

#echo "postun of %{name}-%{version}-%{release}: count=$1"

# If this is an actual removal and not an upgrade,
if [ $1 -lt 1 ]
	then
	#echo "  Is an actual removal, not an upgrade."

	# Tell Xinetd to reload its configuration files.
	killall -HUP xinetd 2>/dev/null

	# Remove the PPR users and groups.
	/usr/sbin/userdel ppr
	/usr/sbin/userdel pprwww
	/usr/sbin/groupdel ppr
	fi

true

# end of file
