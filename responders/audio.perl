#! @PERL_PATH@ -v
#
# mouse:~ppr/src/responders/audio.perl
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
# Last modified 17 January 2005.
#

# Fool Perl so it will not complain about our already secure PATH and IFS.
# This is necessary because this script will sometimes be run from ppr
# rather than pprd.	 In that case, the real and effective user ids will
# probably be different.  That difference triggers Perl's taint checking
# mechanism.
$ENV{'PATH'} =~ /^(.*)$/;
$ENV{'PATH'} = $1;
$ENV{'IFS'} =~ /^(.*)$/;
$ENV{'IFS'} = $1;

use lib "@PERL_LIBDIR@";
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

