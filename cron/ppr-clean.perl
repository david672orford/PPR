#! @PERL_PATH@
#
# mouse:~ppr/src/cron/ppr-clean.perl
# Copyright 1995--2006, Trinity College Computing Center.
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
# Last modified 3 April 2006.
#

#
# This program is run daily from Cron in order to remove temporary files which
# the PPR programs have left lying around.  Of course, it would be better to 
# eliminate this behaviour, but this program is actually a help toward that
# end since it reports on infractions.
#

$LIBDIR="@LIBDIR@";
$VAR_SPOOL_PPR="@VAR_SPOOL_PPR@";
$CONFDIR="@CONFDIR@";
$TEMPDIR="@TEMPDIR@";
$PRINTERS_PURGABLE_STATEDIR="@PRINTERS_PURGABLE_STATEDIR@";

$opt_debug = 1;
$opt_all_removable = 0;

# This is a chatty wrapper around unlink().
sub remove
	{
	$file = shift;
	print "    remove(\"$file\")\n" if($opt_debug);
	if(!unlink($file))
		{ print "Can't remove \"$file\", $!\n" }
	}

# The file named in the first argument is removed if it is older than the 
# number of days stated in the second.
sub remove_if_old
	{
	$file = shift;
	$reference_age = shift;
	print "  remove_if_old(\"$file\", $reference_age):" if($opt_debug);
	if(-M $file > $reference_age)
		{
		print " removing..." if($opt_debug);
		if(!unlink($file))
			{
			print " $!";
			}
		print "\n" if($opt_debug);
		}
	else
		{
		print " not removed\n" if($opt_debug);
		}
	}

# This calls either remove_if_old() or just remove(), depending
# on whether --all-removable was used.
sub remove_switch
	{
	my $file = shift;
	my $reference_age = shift;
	if($opt_all_removable)
		{
		remove($file);
		}
	else
		{
		remove_if_old($file, $reference_age);
		}
	}

# This calls remove_switch() on all files in a directory that match
# an (optional) regular expression.
sub sweepdir
	{
	my $dir = shift;
	my $regexp = shift;
	my $reference_age = shift;

	print "sweepdir(\"$dir\", /$regexp/, $reference_age)\n" if($opt_debug);

	opendir(SWDIR, $dir) || die "Can't open diretory \"$dir\", $!";

	if(defined($regexp))
		{
		while(defined($file = readdir(SWDIR)))
			{
			next if(-d $file);
			if($file =~ /$regexp/)
				{
				remove_switch("$dir/$file", $reference_age);
				}
			}
		}
	else
		{
		while(defined($file = readdir(SWDIR)))
			{
			next if(-d $file);
			remove_switch("$dir/$file", $reference_age);
			}
		}

	closedir(SWDIR) || die $!;
	}

# Command line parsing.
foreach my $item (@ARGV)
	{
	if($item eq "--debug")
		{
		$opt_debug = 1;
		}
	elsif($item eq "--all-removable")
		{
		$opt_all_removable = 1;
		}
	else
		{
		die $item;
		}
	}

# Simple cases
sweepdir($TEMPDIR, '^ppr-', 0.5);
sweepdir($TEMPDIR, '^uprint-', 0.5);
sweepdir("$TEMPDIR/ppr-ipp", undef, 0.5);
sweepdir("$CONFDIR/printers", '^\.ppad\d+$', 0.5);
sweepdir("$CONFDIR/groups", '^\.ppad\d+$', 0.5);
sweepdir("$VAR_SPOOL_PPR/sambaspool", undef, 0.5);
sweepdir("$VAR_SPOOL_PPR/pprclipr", undef, 0.5);
sweepdir("$VAR_SPOOL_PPR/followme.db", undef, 90.0);

opendir(DIR, $PRINTERS_PURGABLE_STATEDIR) || die $!;
while(my $dir = readdir(DIR))
	{
	next if($dir =~ /^\./);
	sweepdir("$PRINTERS_PURGABLE_STATEDIR/$dir", undef, 7.0);
	}
closedir(DIR) || die $!;

# This program is invoked with the --all-removable when we are
# uninstalling PPR.
if($opt_all_removable)
	{
	# Remove boring log files.  Notice that printlog isn't in this list.
	foreach my $l (qw(pprd pprd.old
		pprdrv
		papsrv papd
		olprsrv lprsrv
		ppr-indexfonts ppr-indexppds ppr-indexfilters ppr-clean
		ppr-httpd
		uprint
		))
		{
		my $f = "$VAR_SPOOL_PPR/logs/$l";
		if(-f $f)
			{
			unlink($f) || die "unlink(\"$f\") failed, $!";
			}
		}

	# Remove print jobs.
	sweepdir("$VAR_SPOOL_PPR/queue", undef, 0.0);
	sweepdir("$VAR_SPOOL_PPR/jobs", undef, 0.0);

	# Remove all resource cache files.
	{
	my $dir = "$VAR_SPOOL_PPR/cache";
	opendir(CADIR, $dir) || die "Can't open diretory \"$dir\", $!";
	while(defined($file = readdir(CADIR)))
		{
		next if($file =~ /^\./);
		next if(! -d "$dir/$file");
		sweepdir("$dir/$file", undef, 0.0);
		}
	closedir(CADIR) || die $!;
	}

	# Remove pprd's FIFO.
	unlink("$VAR_SPOOL_PPR/PIPE");

	# Remove any lingering run state files.
	sweepdir("$VAR_SPOOL_PPR/run", undef, 0.0);

	# Remove all of the indexes.
	system("$BINDIR/ppr-index --delete");

	# Remove all of the Samba configuration and converted PPD files.
	#system("$BINDIR/ppr2samba --remove");
	#system("$BINDIR/ppd2macosdrv --remove");

	exit 0;
	}

#
# This is a harder case.  We scan the jobs directory looking for job files
# that don't have cooresponding queue files.  If we find one and it is 
# more than 12 hours old we remove it.	We don't remove newer ones because
# they might be jobs in the process of being deposited in the queue
# directories.
#
print "Scanning \"$VAR_SPOOL_PPR/jobs\"...\n";
opendir(DIR, "$VAR_SPOOL_PPR/jobs") || die "Can't open directory \"$VAR_SPOOL_PPR/jobs\", $!";
while(defined($file = readdir(DIR)))
	{
	if($file =~ /^(.*)-[a-z]+$/)
		{
		my $full_path = "$VAR_SPOOL_PPR/jobs/$file";
		if(-M $full_path > 0.5 && ! -f "$VAR_SPOOL_PPR/queue/$1")
			{
			print "  remove(\"$full_path\")\n";
			remove($full_path);
			}
		}
	}
closedir(DIR) || die $!;

# Done
exit 0;
