#! /usr/bin/perl
#
# mouse:~ppr/src/misc/atalkmapd.perl
# Copyright 1995--1999, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is" without
# express or implied warranty.
#
# Last modified 17 November 1999.
#

#
# This programs uses CAP's getzones and atlook to build a map
# of all the names of type "macUser" on the network.  This program
# reads the configuration file "/etc/ppr/atalk.conf".
#

# You will have to fill these in yourself since
# this program isn't mentioned in the Makefile.
$CONFDIR="?";
$VAR_SPOOL_PPR="?";
$TEMPDIR="?";

# The directory where we keep the AppleTalk map
$MAPDIR="$TEMPDIR/atmap";

# Our log file
$LOGFILE="$VAR_SPOOL_PPR/logs/atalkmapd";

#
# Spawn a process, capturing the output.
# We can read the output from the file
# handle "OUTPUT".
#
sub run_data_command
    {
    $pid = open(OUTPUT,"-|");

    if( ! defined($pid) )
	{
	die "Fork() failed";
	}
    elsif($pid == 0)			# child
	{
	exec(@_);
	exit(242);
	}

    }

#=========================================================================
# Main procedure
#=========================================================================

#
# Open the configuration file and read the names we will look for
#
open(ACONF,"< $CONFDIR/atalk.conf")
	|| die "atalkmapd: \"$CONFDIR/atalk.conf\" is missing\n";

for($x=0; $line=<ACONF>; )
    {
    next if( $line =~ /^#/ );

    if( $line =~ /^\$CAPBIN\s*=\s*["]*([^ \t\n"]+)/ )
	{
	$CAPBIN=$1;
	}

    if( $line =~ /^([^ \t]+)[ \t]+[^ \t]+/ )
	{
	$types[$x]=$1;
	$x++;
	}
    }

close(ACONF) || die;

#
# Make sure we have what we need to find getzones
#
die "atalkmapd: \$CAPBIN not defined in \"$CONFDIR/atalk.conf\"\n"
	if( ! defined($CAPBIN) );

die "atalkmapd: \"$CAPBIN/getzones\" doesn't exist\n"
	if( ! -x "$CAPBIN/getzones" );

die "atalkmapd: \"$CAPBIN/atlook\" doesn't exist\n"
	if( ! -x "$CAPBIN/atlook" );

#
# Remove any old AppleTalk map and create a new one
#
system("rm -rf $MAPDIR; mkdir $MAPDIR");

#
# Become a daemon
#
open(STDIN,"< /dev/null") || die;
open(STDERR,"> $LOGFILE") || die;
select(STDERR);
$|=1;
open(STDOUT,">&STDERR") || die;
select(STDOUT);
$|=1;
exit(0) if( fork() );

#
# Get a zone list
#
&run_data_command("$CAPBIN/getzones");
for($x=0; $_=<OUTPUT>; $x++)
    { chop; $zones[$x]=$_; }
close(OUTPUT);

#
# Start of infinite main loop
#
for($iteration=0; 1; $iteration++)
    {
    #
    # Search each zone in turn
    #
    foreach $zone (@zones)
	{
	print "Searching \"$zone\"\n";

	# Open a file to hold the new zone listing
	open(MAP,">$MAPDIR/.new") || die;

	# Search for each name type
	foreach $type (@types)
	    {
	    print "\tfor \"$type\"";
	    $count=0;

	    # Use the CAP atlook program to get a zone listing
	    &run_data_command("$CAPBIN/atlook","=:$type\@$zone");

	    $junk1=<OUTPUT>;		# Throw away abInit() output
	    $junk2=<OUTPUT>;		# Throw away "Looking for"

	    if( $junk1 !~ /^abInit/ || $junk2 !~ /^Looking/ )
		{
		print "\$junk1 = \"$junk1\"\n";
		print "\$junk2 = \"$junk2\"\n";
		}

	    # Parse the data we want out of the out lines
	    while(<OUTPUT>)
	        {
	        if( $_ =~ /^[ \d]{3} - ([^:]+):([^@]+)[^[]*\[Net:\s*(\d+)\.(\d+)\s*Node:\s*(\d+).*\n$/ )
		    {
		    $name = $1;
		    $type = $2;
		    $net = $3 * 256 + $4;
		    $node = $5;

		    # print "$net:$node, name=\"$name\", $type=\"$type\"\n";

		    print MAP "$net:$node:$name:$type\n";

		    $count++;
		    }
	        }

	    close(OUTPUT) || die "failed to close OUTPUT, $!\n";
	    if($? != 0)
		{
		die "Child error: $?\n";
		}

	    print ", $count found\n";
	    }

	# Close the new map file and move it into place
	close(MAP);
	unlink("$MAPDIR/$zone");
	rename("$MAPDIR/.new","$MAPDIR/$zone");

	# This sleep is here so we will not generate
	# too much network traffic.
	if($iteration > 0)
	    {
	    print "pausing\n";
	    sleep(60);
	    }
	}

    } # end of main loop which never willingly ends

# end of file

