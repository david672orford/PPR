#! /usr/bin/perl
#
# mouse:~ppr/src/misc/sgi_glp_hack.perl
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
# Last modified 5 April 2003.
#

#
# This script is meant for use with the uprint_* programs
# on SGI programs.	Some SGI programs use a print dialog
# box which gets a list of printers by examining certain
# files and directories in /var/spool/lp.  This script
# creates dummy entries in /var/spool/lp for all of the
# PPR printers and any printers named in uprint.conf.
#

# Directory where each printer has a file which
# contains its port name:
$PORT_DIR = "/var/spool/lp/member";

# Directory where each printer has an interface
# program:
$INTERFACE_DIR = "/var/spool/lp/interface";

# PPR configuration directory:
$CONFDIR = "/etc/ppr";

# Uprint configuration file:
$UPRINT_CONF = "$CONFDIR/uprint-remote.conf";

# PPR printer and group configuration directories:
$PPR_PRINTERS = "$CONFDIR/printers";
$PPR_GROUPS = "$CONFDIR/groups";

# List of uprint printers found:
%OK_LIST = ();

#
# Create dummy port and interface files
# for a uprint printer.
#
sub do_dest
	{
	my $dest = shift;

	print "  Adding \"$dest\".\n";

	$OK_LIST{$dest} = 1;

	open(PORT, ">$PORT_DIR/$dest") || die;
	print PORT "/dev/null\n";
	close(PORT);

	open(INT, ">$INTERFACE_DIR/$dest") || die;
	print INT <<"End";
#UPRINT
#
# This interface script is a dummy created by
# by the PPR uprint system so that glp and
# friends will work.  Do not change the first
# line, if you do, uprint will not know that it
# is safe to delete this script when it is no longer
# needed.
#
End

	close(PORT);
	}

#
# Scan a directory and consider each file
# to be a queue configuration.
#
sub do_dir
	{
	my $dir = shift;
	my $file;

	print "Searching \"$dir\":\n";

	opendir(D, $dir) || die;
	while($file = readdir(D))
		{
		next if($file =~ /^\./);
		next if($file =~ /~$/);
		if(-f "$dir/$file")
			{ do_dest($file) }
		}
	closedir(D);
	}

#
# Start
#
print "Updating fake lp queues:\n";

($> == 0) || die "You are not root!\n";

#
# Do PPR queues:
#
-d $PPR_PRINTERS && do_dir($PPR_PRINTERS);
-d $PPR_GROUPS && do_dir($PPR_GROUPS);

#
# Add all the printers in uprint.conf:
#
print "Searching \"$UPRINT_CONF\":\n";
if(open(U, "<$UPRINT_CONF"))
	{
	while(<U>)
		{
		if(/^\[([^]]+)\]/)
			{
			do_dest($1);
			}
		}
	close(U);
	}

#
# Remove those that are no longer needed:
#
print "Removing old queues:\n";
opendir(D, $INTERFACE_DIR) || die "Can't search \"$INTERFACE_DIR\", $!.\n";
my $dir;
while($dir = readdir(D))
	{
	next if($dir =~ /^\./);

	if(! defined($OK_LIST{$dir}))
		{
		# Examine the interface.
		open(I, "<$INTERFACE_DIR/$dir") || die "Can't open \"$INTERFACE_DIR/$dir\", $!.\n";
		my $line1 = <I>;
		close(I);

		# If it is one of our's, delete it.
		if($line1 eq "#UPRINT\n")
			{
			print "  Removing \"$dir\".\n";
			unlink("$INTERFACE_DIR/$dir") || die "Can't delete \"$INTERFACE_DIR/$dir\", $!.\n";
			if( ! unlink("$PORT_DIR/$dir"))
				{ print "\tFailed to remove \"$PORT_DIR/$dir\"!\n" }
			}
		}
	}
closedir(D);

print "Done.\n";
exit 0;

