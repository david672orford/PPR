#! /usr/bin/perl
#
# mouse:~ppr/src/init_and_cron/ppr-clean.perl
# Copyright 1995--2002, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is" without
# express or implied warranty.
#
# Last modified 26 August 2002.
#

#
# This program is designed to remove temporary files which the
# PPR programs have left lying around.  Of course, it would be
# better to eliminate this behaviour, but this program is actually
# a help toward that end since it reports on infractions.
#

$HOMEDIR = "?";
$VAR_SPOOL_PPR = "?";
$CONFDIR = "?";
$TEMPDIR = "?";

$debug = 1;

sub remove
    {
    $file = shift;
    print "    remove(\"$file\")\n" if($debug);
    if(!unlink($file))
    	{ print "Can't remove \"$file\", $!\n" }
    }

sub remove_if_old
    {
    $file = shift;
    $reference_age = shift;
    print "  remove_if_old(\"$file\", $reference_age)\n" if($debug);
    unlink($file) if(-M $file > $reference_age);
    }

sub sweepdir
    {
    $dir = shift;
    $regexp = shift;
    $reference_age = shift;

    print "sweepdir(\"$dir\", /$regexp/, $reference_age)\n" if($debug);

    opendir(DIR, $dir) || die "Can't open diretory \"$dir\", $!";
    if(defined($regexp))
	{
	while(defined($file = readdir(DIR)))
    	    {
	    next if(-d $file);
	    if($file =~ /$regexp/)
		{
		remove_if_old("$dir/$file", $reference_age);
		}
    	    }
    	}
    else
	{
	while(defined($file = readdir(DIR)))
    	    {
	    next if(-d $file);
    	    remove_if_old("$dir/$file", $reference_age);
    	    }
    	}
    closedir(DIR);
    }

# Simple cases
sweepdir($TEMPDIR, '^ppr-$', 0.5);
sweepdir($TEMPDIR, '^uprint-$', 0.5);
sweepdir("$CONFDIR/printers", '^\.ppad\d+$', 0.5);
sweepdir("$CONFDIR/groups", '^\.ppad\d+$', 0.5);
sweepdir("$VAR_SPOOL_PPR/sambaspool", undef, 0.5);
sweepdir("$VAR_SPOOL_PPR/pprclipr", undef, 0.5);
sweepdir("$VAR_SPOOL_PPR/printers/alerts", undef, 7.0);
sweepdir("$VAR_SPOOL_PPR/pprpopup.db", undef, 0.5);

# All resource caching directories:
opendir(CACHE, "$VAR_SPOOL_PPR/cache") || die "Can't open directory \"$VAR_SPOOL_PPR/cache\", $!";
while(defined(<CACHE>))
    {
    next if(/^\./);
    next if(! -d);
    sweepdir("$VAR_SPOOL_PPR/$_", '^\.temp\d+$', 0.5);
    }
closedir(CACHE) || die;

# This is a harder case
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
closedir(DIR);

# Done
exit 0;

# end of file

