#! /usr/bin/perl
#
# mouse:~ppr/src/responders/atalk.perl
# Copyright 1995--2001, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is" without
# express or implied warranty.
#
# Last modified 3 April 2001.
#

#
# Theoretically, this program is not directly dependent on CAP.
# However, as currently written it requires CAP.
#
# This program uses an AppleTalk map created by atalkmapd (in the
# misc directory) to convert network:node addresses to NAME:TYPE@ZONE.
#
# Once it has found the socket number in the AppleTalk map, this
# responder executes a program to send a message to a Macintosh
# program such as "Messages" (which comes with CAP60).  The programs
# which may be used to dispatch the message are defined in
# /etc/ppr/atalk.conf.
#
# This responder has never worked very well.
#

# These are filled in by installscript.
$CONFDIR="?";
$TEMPDIR="?";
$VAR_SPOOL_PPR="?";

# The directory where we store the AppleTalk map.
$MAPDIR="$TEMPDIR/atmap";

# Log file for when $DEBUG > 0
$LOGFILE="$VAR_SPOOL_PPR/logs/responder_atalk";

#
# Spawn a process and wait for it.
#
sub run
    {
    $pid=fork();

    if($pid == -1)			# error
	{				# not much we can do
	die "Fork() failed\n";
	}
    elsif($pid == 0)			# child
	{				# exec the program
	exec(@_);
	exit(242);
	}
    else				# parent
	{				# wait for child to exit
	wait;
	}
    } # end of run()

#======================================================================
# Start of Main, parse the command line
#======================================================================

# The "For:" line from the PostScript document.
# We will use this to say "Message for XXXX".
$FOR=$ARGV[0];

# Address is in the form "net:node".
# We will split it up immediately.
($addr_net,$addr_node)=split(/:/,$ARGV[1]);

# Change line feeds to spaces since most Macintosh message
# programs do word wrap.
($MESSAGE=$ARGV[2]) =~ tr/\n/ /;

#===================================================================
# Parse the configuration file.
# Even if $DEBUG is non-zero, debugging isn't on yet, so nobody
# is likely to see the error messages which may be printed in this
# section.
#===================================================================
$DEBUG=0;

open(ACONF,"< $CONFDIR/atalk.conf")
	|| die "atalkmapd: \"$CONFDIR/atalk.conf\" is missing\n";

for($x=0; $line=<ACONF>; )
    {
    next if( $line =~ /^#/ );

    if( $line =~ /^\$CAPBIN\s*=\s*["]*([^ \t\n"]+)/ )
	{
	$CAPBIN=$1;
	}

    elsif( $line =~ /^\$DEBUG\s*=\s*([0-9]+)/ )
	{
	$DEBUG=$1;
	}

    elsif( $line =~ /^([^ \t]+)[ \t]+([^\n]+)/ )
	{
	$methods{$1}=$2;
	$x++;
	}
    }

close(ACONF) || die;

# If debuging, open the debug file and write a start line
if( $DEBUG > 0 )
    {
    open(STDOUT,">> $LOGFILE") || die;
    open(STDERR,">&STDOUT") || die;
    print "atalk \"$ARGV[0]\" \"$ARGV[1]\" \"$ARGV[2]\" $ARGV[3]\n";
    }

# Check to make sure $CAPBIN was defined.  I don't suppose anyone
# will be able to read this message.
die "atalk responder: \$CAPBIN not defined in \"$CONFDIR/atalk.conf\"\n"
	if( ! defined($CAPBIN) );

#=========================================================================
# Look for a message receiver on the node we want and eval the code
# for the first one we find.
#=========================================================================

# Search each zone in the zone map directory
opendir(ZONES,$MAPDIR) || die "Atalk map directory \"$MAPDIR\" does not exist\n";

# Go thru the zones until the end or we
# find the machine we are looking for.
$machine_not_found_yet=1;
while( $machine_not_found_yet && ($ZONE=readdir(ZONES)) )
    {
    # Skip hidden files and the current, parent entries
    if( $ZONE =~ /^\./ ) { next; }

    # Open this list for this zone
    open(ZONEFILE,"<$MAPDIR/$ZONE") || die;

    while(<ZONEFILE>)
	{
	chop;

	($net,$node,$NAME,$TYPE)=split(/:/);

	if( ($net==$addr_net) && ($node==$addr_node) )
	    {				# Having found machine in this zone,
	    $machine_not_found_yet=0;	# won't find it in other zones.

	    if( defined( ($command=$methods{$TYPE}) ) )
		{
		print "Running $command\n" if( $DEBUG > 0 );
		eval $command;
		print "$@\n" if( $@ ne "");
		exit(0);
		}

	    }
	} # end of for each name in zone

    close(ZONEFILE);
    } # end of for each zone

closedir(ZONES);

# If we get this far we have failed, but don't wory about it.
if( $DEBUG > 0 )
    {
    if( $machine_not_found_yet )
	{ print "Machine $addr_net:$addr_node not found in network map.\n"; }
    else
	{ print "No suitable program found on client.\n"; }
    }

exit(0);

# end of file

