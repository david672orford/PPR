#
# mouse:~ppr/src/libscript/speach_response.pl
# Copyright 1995--2000, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appears in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is"
# without express or implied warranty.
#
# Last modified 16 November 2000.
#

# Pull in the definitions of $RESP_*.
require 'respond.ph';

# The various job response messages:
my %msg;
$msg{$RESP_FINISHED} = ["has been printed on", "X"];
$msg{$RESP_ARRESTED} = ["has been arrested"];
$msg{$RESP_CANCELED} = ["has been canceled"];
$msg{$RESP_CANCELED_PRINTING} = ["was canceled while printing on", "X"];
$msg{$RESP_CANCELED_BADDEST} = ["has been discarded", "because", "the destination", "X", "does not exist"];
$msg{$RESP_CANCELED_REJECTING} = ["has been discarded", "because", "the destination", "X", "is not accepting jobs at this time"];
$msg{$RESP_CANCELED_NOCHARGEACCT} = ["has been discarded", "because", "you do not have a printing charge account"];
$msg{$RESP_CANCELED_BADAUTH} = ["has been discarded", "because", "your password was wrong"];
$msg{$RESP_CANCELED_OVERDRAWN} = ["has been discarded", "because", "your account is overdrawn"];
$msg{$RESP_STRANDED_PRINTER_INCAPABLE} = ["is stranded", "because", "the printer", "X", "is incapable of printing it"];
$msg{$RESP_STRANDED_GROUP_INCAPABLE} = ["is stranded", "because", "none of the printers in the group", "X", "is capable of printing it"];
$msg{$RESP_CANCELED_NONCONFORMING} = ["has been discarded", "because", "it does not conform to the document structuring convention"];
$msg{$RESP_NOFILTER} = ["has been discarded", "because", "no filter is available to convert it to postscript"];
$msg{$RESP_FATAL} = ["has been discarded", "because", "of a fatal ppr error"];
$msg{$RESP_NOSPOOLER} = ["has been discarded", "because", "the spooler is not running"];
$msg{$RESP_BADMEDIA} = ["has been discarded", "because", "the medium you have requested is not recognized"];
$msg{$RESP_BADPJLLANG} = ["has been discarded", "because", "the pjl header requests an unrecognized printer language"];
$msg{$RESP_FATAL_SYNTAX} = ["has been discarded", "because", "there is an error in the ppr command line"];
$msg{$RESP_CANCELED_NOPAGES} = ["has been discarded", "because", "your order to print only selected pages can't be fulfilled"];
$msg{$RESP_CANCELED_ACL} = ["has been discarded", "because", "you are not authorized to print on", "X"];

sub speach_ppr_response
{
my($args, $silly_sounds) = @_;

# Start to build a list of words and phrases to be played.
my @playlist;

my($destnode, $destname, $id, $subid, $homenode) = split(/ /, $args->{JOBID});

if($id != 0)							# if job had an id assigned,
  {
  # Start with an almost empty playlist:
  @playlist = ('your print job');

  # Add the print job name:
  speach_spellout(\@playlist, $destname);
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
if(defined($args->{TIME}) && $args->{TIME} =~ /^\d+$/)
	{
	my $elapsed_seconds = (time() - $args->{TIME});
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
if( defined($message = $msg{$args->{CODE}}) )
  { push(@playlist, @$message) }
else
  { push(@playlist, "?") }

# See if we can add to it:
if($args->{CODE} == $RESP_FINISHED && $args->{PAGES} ne "?")
  {
  push(@playlist, "(pause)");
  push(@playlist, "the document is");

  if($args->{PAGES} == 1)
	{
	push(@playlist, "one page long");
	}
  else
	{
	speach_number(\@playlist, $args->{PAGES});
	push(@playlist, "pages long");
	}
  }

# Replace "X" in the playlist with the spelled out extra parameter:
for($x=0; $x <= $#playlist; $x++)
  {
  if( $playlist[$x] eq "X" )
	 {
	 @extra = ();
	 speach_spellout(\@extra, $args->{EXTRA});
	 splice(@playlist, $x, 1, @extra);
	 }
  }

return @playlist;
}

1;

