#! /usr/bin/perl
#
# mouse.trincoll.edu:~ppr/src/uprint/sgi_glp_hack.perl
# Copyright 1997, 1998, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is" without
# express or implied warranty.
#
# Last revised 4 September 1998.
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

	print "	 Adding \"$dest\".\n";

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
			print "	 Removing \"$dir\".\n";
			unlink("$INTERFACE_DIR/$dir") || die "Can't delete \"$INTERFACE_DIR/$dir\", $!.\n";
			if( ! unlink("$PORT_DIR/$dir"))
				{ print "\tFailed to remove \"$PORT_DIR/$dir\"!\n" }
			}
		}
	}
closedir(D);

print "Done.\n";
exit 0;

