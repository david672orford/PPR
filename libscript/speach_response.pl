#
# mouse:~ppr/src/libscript/speach_response.pl
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
# Last modified 25 March 2005.
#

# Pull in the definitions of $RESP_*.
require 'respond.ph';

# The various job response messages:
my %msg;
$msg{$RESP_FINISHED} =
	["has been printed on", "PRINTER"];
$msg{$RESP_ARRESTED} =
	["has been arrested"];
$msg{$RESP_CANCELED} =
	["has been canceled"];
$msg{$RESP_CANCELED_PRINTING} =
	["was canceled while printing on", "PRINTER"];
$msg{$RESP_CANCELED_BADDEST} =
	["has been discarded", "because", "the destination", "DESTINATION", "does not exist"];
$msg{$RESP_CANCELED_REJECTING} =
	["has been discarded", "because", "the destination", "DESTINATION", "is not accepting jobs at this time"];
$msg{$RESP_CANCELED_NOCHARGEACCT} = 
	["has been discarded", "because", "you do not have a printing charge account"];
$msg{$RESP_CANCELED_BADAUTH} =
	["has been discarded", "because", "your password was wrong"];
$msg{$RESP_CANCELED_OVERDRAWN} =
	["has been discarded", "because", "your account is overdrawn"];
$msg{$RESP_STRANDED_PRINTER_INCAPABLE} =
	["is stranded", "because", "the printer", "PRINTER", "is incapable of printing it"];
$msg{$RESP_STRANDED_GROUP_INCAPABLE} =
	["is stranded", "because", "none of the printers in the group", "DESTINATION", "is capable of printing it"];
$msg{$RESP_CANCELED_NONCONFORMING} =
	["has been discarded", "because", "it does not conform to the document structuring convention"];
$msg{$RESP_NOFILTER} =
	["has been discarded", "because", "no filter is available to convert it to postscript"];
$msg{$RESP_FATAL} =
	["has been discarded", "because", "of a fatal ppr error"];
$msg{$RESP_NOSPOOLER} =
	["has been discarded", "because", "the spooler is not running"];
$msg{$RESP_BADMEDIA} =
	["has been discarded", "because", "the medium you have requested is not recognized"];
$msg{$RESP_BADPJLLANG} =
	["has been discarded", "because", "the pjl header requests an unrecognized printer language"];
$msg{$RESP_FATAL_SYNTAX} =
	["has been discarded", "because", "there is an error in the ppr command line"];
$msg{$RESP_CANCELED_NOPAGES} =
	["has been discarded", "because", "your order to print only selected pages can't be fulfilled"];
$msg{$RESP_CANCELED_ACL} =
	["has been discarded", "because", "you are not authorized to print on", "DESTINATION"];

sub speach_ppr_response
	{
	my($args, $silly_sounds) = @_;

	# Start to build a list of words and phrases to be played.
	my @playlist;

	my($destination, $id, $subid);
	if(defined $args->{job})
		{
		$args->{job} =~ /^(.+)-(\d+)\.(\d)$/ || die;
		($destination, $id, $subid) = ($1, $2, $3);
		}
	else
		{
		($destination, $id, $subid) = ($args->{destination}, "", 0);
		}

	if($id != 0)							# if job had an id assigned,
		{
		# Start with an almost empty playlist:
		@playlist = ('your print job');

		# Add the print job name:
		speach_spellout(\@playlist, $destination);
		speach_spellout(\@playlist, $id);		# say the digits
		#speach_number(\@playlist, $id);		# or pronounce it in high style
		if($subid != 0)
			{
			push(@playlist, "part");
			speach_number(\@playlist, $subid);
			}
		}
	else									# if no id assigned,
		{
		@playlist = ('your new print job for');
		speach_spellout(\@playlist, $destname);
		}

	# Add a parenthetical statement about when it was submitted:
	if(defined($args->{time}) && $args->{time} =~ /^\d+$/)
		{
		my $elapsed_seconds = (time() - $args->{time});
		if($elapsed_seconds > 0)
			{
			push(@playlist, "which you submitted");
			speach_time_interval(\@playlist, $elapsed_seconds);
			push(@playlist, "ago");
			}
		}

	# Add the main part of the message.	 In the unlikely event
	# that the code is undefined, we will deliberately use a
	# non-existent send in order to make a beep.
	if( defined($message = $msg{$args->{response_code}}) )
		{ push(@playlist, @$message) }
	else
		{ push(@playlist, "?") }

	# See if we can add to it:
	if($args->{response_code} == $RESP_FINISHED && $args->{pages} ne "?")
		{
		push(@playlist, "(pause)");
		push(@playlist, "the document is");

		if($args->{pages} == 1)
			{
			push(@playlist, "one page long");
			}
		else
			{
			speach_number(\@playlist, $args->{pages});
			push(@playlist, "pages long");
			}
		}

	# Replace "X" in the playlist with the spelled an extra parameter.
	for($x=0; $x <= $#playlist; $x++)
		{
		if($playlist[$x] =~ /^([A-Z_]+)$/)
			{
			my $key = $1;
			$key =~ tr/[A-Z]/[a-z]/;
			@extra = ();
			speach_spellout(\@extra, $args->{$key});
			splice(@playlist, $x, 1, @extra);
			}
		}

	return @playlist;
	}

1;

