#! /usr/bin/perl -w
#
# mouse:~ppr/src/commentators/audio.perl
# Copyright 1995-2001, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software is provided "as is" without express or
# implied warranty.
#
# Last modified 4 May 2001.
#

#
# A commentator is invoked like this:
#
# commentators/audio <address> <options> <printer> <code> <cooked_message> <raw1> <raw2> <severity>
#
# This commentator passes <address> to the the routine speach_play_many()
# (in speach_play.pl).	Thus you may use any form of address acceptable
# to that routine.	Currently it supports two forms of address:
#
# chappell.pc.trincoll.edu:15009
#		This address causes the file to be copied to a Samba share area 
#		and a TCP/IP connexion is opened to a MS-Windows computer running
#		pprpopup which is then instructed to play the sound in the share area.	
#
# /dev/dsp
#		This address causes the file to be played in the indicated audio
#		device.
#

# These will be filled in when this script is installed:
$HOMEDIR = "?";
$SHAREDIR = "?";
$VAR_SPOOL_PPR = "?";
$TEMPDIR="?";

use lib "?";
require 'speach.pl';
require 'speach_play.pl';
require 'speach_commentary.pl';

#
# If this is non-zero, debugging messages will be printed on stdout.  Higher
# levels produce more messages.	 Any message written on stdout or stderr will
# find their way into the pprdrv log file.
#
# This will include messages about missing sounds.
#
$DEBUG = 1;

#==============================================================
# Main
#==============================================================

# Assign names to the command line parameters.
my($ADDRESS, $OPTIONS, $PRINTER, $CODE, $COOKED, $RAW1, $RAW2, $SEVERITY) = @ARGV;

my $severity_threshold = 1;
my $silly_sounds = 0;

# Parse the options parameter which is a series of name=value pairs.
my $option;
foreach $option (split(/[\s]+/, $OPTIONS))
  {
  my($name, $value) = split(/=/, $option);

  if($name eq "level")
	{
	$severity_threshold = $value;
	}
  elsif( $name eq "voice" )
	{
	speach_set_voice($value);
	}
  elsif( $name eq "silly_sounds" )
	{
	if($value =~ /^[1ty]/i)
		{ $silly_sounds = 1 }
	else
		{ $silly_sounds = 0 }
	}
  else
	{
	die "Audio commentator: unrecognized option: $name\n";
	}
  }

# Debugging code:
if($DEBUG > 1)
  {
  $| = 1;
  print "\$ADDRESS=\"$ADDRESS\", \$OPTIONS=\"$OPTIONS\", \$PRINTER=$PRINTER, \$CODE=$CODE,\n";
  print "\$COOKED=\"$COOKED\", \$RAW1=\"$RAW1\", \$RAW2=\"$RAW2\"\n";
  print "\$serverity_threshold=$severity_threshold, \$voice=\"$voice\", \$silly_sounds=$silly_sounds\n";
  }

if($SEVERITY < $severity_thresold)
  {
  print "audio commentator:	 not not important enough to play.\n" if($DEBUG > 1);
  exit 0;
  }

my @playlist = speach_ppr_commentary($PRINTER, $CODE, $COOKED, $RAW1, $RAW2, $SEVERITY, $silly_sounds);

# Print the message for debugging purposes:
print join(' ', @playlist), "\n" if($DEBUG > 2);

# Play the sound.
speach_play_many($ADDRESS, @playlist);

# We are done.
print "Done\n\n" if($DEBUG > 1);
exit(0);

# end of file

