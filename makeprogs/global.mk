#
# ~ppr/src/makeprogs/global.mk
# Generated by Configure on Thu Jun 21 17:19:10 EDT 2001.
#
# You may want to change some of this if it doesn't work on
# your system.
#

# This name of the system on which PPR will be compiled.
# You should change this if you are creating a new port.
PORT_NAME=LINUX

# Directory for program files and such.
# Default is /usr/lib/ppr.
HOMEDIR=/usr/lib/ppr

# Directory for architecture independent files
# Default is /usr/share/ppr
SHAREDIR=/usr/share/ppr

# Directory into which to install configuration files.
# Default is /etc/ppr.
CONFDIR=/etc/ppr

# Directory for spool files.
# Default is /var/spool/ppr.
VAR_SPOOL_PPR=/var/spool/ppr

# Directory for temporary files.
# Default is /tmp.
TEMPDIR=/tmp

# Directory for symbolic links to PPR executables
# Default is /usr/bin.
BINDIR=/usr/bin

# Directory were we will find X-Windows programs
# Default is /usr/bin/X11.
XWINBINDIR=/usr/bin/X11

# Directory for HTML files.  If you change this
# you will have to fix makeprogs/make_install_dirs.sh too.
WWWDIR=$(SHAREDIR)/www

# Directory for man pages to be installed in.
MANDIR=$(SHAREDIR)/man

# Directory for the CGI programs for the WWW interface.
CGI_BIN=$(HOMEDIR)/cgi-bin

# If all of the above have their default values then,
# this line may be commented out.
#CHANGE=-DHOMEDIR=\"$(HOMEDIR)\" -DSHAREDIR=\"$(SHAREDIR)\" -DCONFDIR=\"$(CONFDIR)\" -DVAR_SPOOL_PPR=\"$(VAR_SPOOL_PPR)\" -DTEMPDIR=\"$(TEMPDIR)\" -DXWINBINDIR=\"$(XWINBINDIR)\"

# Do we want international messages support?
# This uses GNU Gettext or Solaris Gettext.
#
# Notice that INTLLIBS is set to nothing.  The commented
# out value is sometimes needed, such as when you build
# GNU gettext yourself so that it is not integrated into
# the C library, but on RedHat 5.1 systems which have been
# upgraded from certain earlier versions the /usr/lib/libintl.a
# is poison.  That is why it is commented out by default.
INTERNATIONAL=-DINTERNATIONAL
INTLLIBS=#-lintl
INTERNATIONAL_INSTALL=install-international

# Define the owner and group of most PPR files.  If any of USER_PPR,
# GROUP_PPR, or USER_PPRWWW are defined as something other than the
# defaults, then the USER_GROUP= line must be uncommented.
USER_PPR=ppr
GROUP_PPR=ppr
USER_PPRWWW=pprwww
#USER_GROUP=-DUSER_PPR=\"$(USER_PPR)\" -DGROUP_PPR=\"$(GROUP_PPR)\" -DUSER_PPRWWW=\"$(USER_PPRWWW)\"

# Shell script which modifies shell and Perl scripts to use the above values.
# Don't change this setting.
SCRIPTFIXUP=../makeprogs/scriptfixup.sh

# A shell script which copies executable files into place.  It also attempts
# to strip them.  We don't use "install" because the BSD and System V versions
# are very different.  There is no reason to change this setting.
INSTALLPROGS=../makeprogs/installprogs.sh

# A shell script which copies data files into place.  There is no reason to
# change this setting.
INSTALLDATA=../makeprogs/installdata.sh

# This copies a single file into place, renaming it as it goes.
INSTALLCP=../makeprogs/installcp.sh

# This links a single file to another.
INSTALLLN=../makeprogs/installln.sh

# Shell script which creates a directory, deleting any
# old one first.
MAKE_NEW_DIR=../makeprogs/make_new_dir.sh

# Program to rebuild .depend
PPR_MAKE_DEPEND=../makeprogs/ppr_make_depend.perl

# Programs for uncompressing files.  This variable is used when
# building ppr/ppr_infile.c.
UNCOMPRESS_OPTS=-DGUNZIP=\"/bin/gunzip\"

# Echo programs that support -n and backslash escapes.
NECHO=echo
EECHO=echo -e

# Location of Perl interpreter.
PERL=/usr/local/bin/perl

# Decide which user database module to use
USER_DBM=gdbm
#USER_DBM=none

# Extra libraries which should be included when linking to the PPR user
# database library.
DBLIBS=-lgdbm
#DBLIBS=

# Wrap several options together into one
MISC_OPTS=-DPPR_$(PORT_NAME) $(CHANGE) $(USER_GROUP) $(INTERNATIONAL)

# Backup files to delete
BACKUPS=*~ *.bak *.bck

# Here are some defaults for the system dependent section:
MV=mv
RMF=rm -f
CHMOD=chmod

#============================================================
# System Dependent Section
# For: Linux
#============================================================
MAKE=make --no-print-directory
STRIP=strip
OBJ=o
DOTEXE=
LIBEXT=a
LIBCMD=ar -crs
CC=gcc
#CC=diet
#CC=g++ -fno-exceptions
CFLAGS=-Wall -Wpointer-arith -Wmissing-declarations -O2 -mcpu=i686 -fomit-frame-pointer -I ../include $(MISC_OPTS)
LD=gcc
#LD=diet
LDFLAGS=
CPP=gcc -E -P -xc-header
SOCKLIBS=
LEX=flex
LEX_LIB=
PARALLEL=linux
#==============================================================
# Make Rules
# We use these to reduce the complexity of the makefiles.  We
# also can't really depend of different versions of make to
# have the same internal rules.
#==============================================================

.SUFFIXES: .sh .perl .c .$(OBJ)

# Convert .sh files to shell scripts.
.sh:
	$(SCRIPTFIXUP) $*.sh $*

# Convert .perl files to Perl scripts.
.perl:
	$(SCRIPTFIXUP) $*.perl $*

# Build an object file from a single .c file.
.c.$(OBJ):
	$(CC) $(CFLAGS) -c $*.c


#==============================================================
# Start of AppleTalk settings.
#==============================================================
# Which AppleTalk library are we using, if any?
ATALKTYPE=ali
ATALKFLAGS=-I /usr/local/include -I /usr/local/atalk/include
ATALKLIBS=/usr/local/lib/libnatali.a /usr/local/atalk/lib/libatalk.a

# Enable building and installing of programs that require AppleTalk.
ALLATALK=allatalk
INSTALLATALK=installatalk


# end of file

