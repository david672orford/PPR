#! @PERL_PATH@ -w
#
# mouse:~ppr/src/commentators/audio.perl
# Copyright 1995--2005, Trinity College Computing Center.
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
# Last modified 13 January 2005.
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
$HOMEDIR="@HOMEDIR@";
$SHAREDIR="@SHAREDIR@";
$VAR_SPOOL_PPR="@VAR_SPOOL_PPR@";
$TEMPDIR="@TEMPDIR@";

use lib "@PERL_LIBDIR@";
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
  print "audio commentator: not not important enough to play.\n" if($DEBUG > 1);
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

