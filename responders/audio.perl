#! /usr/bin/perl -v
#
# mouse:~ppr/src/responders/audio.perl
# Copyright 1995--2001, Trinity College Computing Center.
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

# Fool Perl so it will not complain about our already secure PATH and IFS.
# This is necessary because this script will sometimes be run from ppr
# rather than pprd.  In that case, the real and effective user ids will
# probably be different.  That difference triggers Perl's taint checking
# mechanism.
$ENV{'PATH'} =~ /^(.*)$/;
$ENV{'PATH'} = $1;
$ENV{'IFS'} =~ /^(.*)$/;
$ENV{'IFS'} = $1;

# These will be filled in when this script is installed:
$HOMEDIR = "?";
$SHAREDIR = "?";
$VAR_SPOOL_PPR = "?";
$TEMPDIR = "?";

use lib "?";
require 'responder_argv.pl';
require 'speach.pl';
require 'speach_play.pl';
require 'speach_response.pl';

# If this is non-zero, then debugging messages will be sent to stdout.
# Such messages will either appear on the terminal from which ppr is run
# or in the pprd log file.
$DEBUG = 1;

# Assign names to our arguments:
my $args = responder_argv(@ARGV);

# Parse the options parameter which is a series of name=value pairs.
$OPTION_PRINTED = 1;
$OPTION_CANCELED = 1;
$OPTION_SILLY_SOUNDS = 0;
my $option;
foreach $option (split(/[\s]+/, $args->{OPTIONS}))
  {
  my($name, $value) = split(/=/, $option);

  if($name eq "printed")
    {
    if($value =~ /^[1-9ty]/io)
    	{ $OPTION_PRINTED = 1 }
    else
    	{ $OPTION_PRINTED = 0 }
    }
  if($name eq "canceled")
    {
    if($value =~ /^[1-9ty]/io)
    	{ $OPTION_CANCELED = 1 }
    else
    	{ $OPTION_CANCELED = 0 }
    }
  elsif($name eq "voice")
    {
    speach_set_voice($value);
    }
  elsif($name eq "silly_sounds")
    {
    if($value =~ /^[ty1-9]/io)
	{ $OPTION_SILLY_SOUNDS = 1 }
    else
	{ $OPTION_SILLY_SOUNDS = 0 }
    }
  }

# If we should not play job done messages and this is one of them, bail out now.
if( ! $OPTION_PRINTED && $CODE == $RESP_FINISHED )
  { exit(0) }

# Same for job canceled messages.
if( ! $OPTION_CANCELED && ($CODE == $RESP_CANCELED || $CODE == $RESP_CANCELED_PRINTING) )
  { exit(0) }

# Get a list of the sound clips to play.
my @playlist = speach_ppr_response($args, $OPTION_SILLY_SOUNDS);

print join(' ', @playlist), "\n" if($DEBUG > 2);

speach_play_many($args->{ADDRESS}, @playlist);

print "Done\n\n" if($DEBUG > 1);

exit(0);

