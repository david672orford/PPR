#! @PERL_PATH@
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
# Last modified 29 March 2005.
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
require 'respond.ph';

# The various job response messages:
my %msg;
$msg{$RESP_FINISHED} =
	["YOUR_PRINTJOB", "WHEN", "has been printed on", "PRINTER", "PAGES"];
$msg{$RESP_ARRESTED} =
	["YOUR_PRINTJOB", "WHEN", "has been arrested"];
$msg{$RESP_CANCELED} =
	["YOUR_PRINTJOB", "WHEN", "has been canceled"];
$msg{$RESP_CANCELED_PRINTING} =
	["YOUR_PRINTJOB", "WHEN", "was canceled while printing on", "PRINTER"];
$msg{$RESP_CANCELED_BADDEST} =
	["YOUR_PRINTJOB", "has been discarded", "because", "the destination", "DESTINATION", "does not exist"];
$msg{$RESP_CANCELED_REJECTING} =
	["YOUR_PRINTJOB", "has been discarded", "because", "the destination", "DESTINATION", "is not accepting jobs at this time"];
$msg{$RESP_CANCELED_NOCHARGEACCT} = 
	["YOUR_PRINTJOB", "has been discarded", "because", "you do not have a printing charge account"];
$msg{$RESP_CANCELED_OVERDRAWN} =
	["YOUR_PRINTJOB", "has been discarded", "because", "your account is overdrawn"];
$msg{$RESP_STRANDED_PRINTER_INCAPABLE} =
	["YOUR_PRINTJOB", "is stranded", "because", "the printer", "PRINTER", "is incapable of printing it"];
$msg{$RESP_STRANDED_GROUP_INCAPABLE} =
	["YOUR_PRINTJOB", "is stranded", "because", "none of the printers in the group", "DESTINATION", "is capable of printing it"];
$msg{$RESP_CANCELED_NONCONFORMING} =
	["YOUR_PRINTJOB", "has been discarded", "because", "it does not conform to the document structuring convention"];
$msg{$RESP_NOFILTER} =
	["YOUR_PRINTJOB", "has been discarded", "because", "no filter is available to convert it to postscript"];
$msg{$RESP_FATAL} =
	["YOUR_PRINTJOB", "has been discarded", "because", "of a fatal ppr error"];
$msg{$RESP_NOSPOOLER} =
	["YOUR_PRINTJOB", "has been discarded", "because", "the spooler is not running"];
$msg{$RESP_BADMEDIA} =
	["YOUR_PRINTJOB", "has been discarded", "because", "the medium you have requested is not recognized"];
$msg{$RESP_BADPJLLANG} =
	["YOUR_PRINTJOB", "has been discarded", "because", "the pjl header requests an unrecognized printer language"];
$msg{$RESP_FATAL_SYNTAX} =
	["YOUR_PRINTJOB", "has been discarded", "because", "there is an error in the ppr command line"];
$msg{$RESP_CANCELED_NOPAGES} =
	["YOUR_PRINTJOB", "has been discarded", "because", "your order to print only selected pages can't be fulfilled"];
$msg{$RESP_CANCELED_ACL} =
	["YOUR_PRINTJOB", "has been discarded", "because", "you are not authorized to print on", "DESTINATION"];
$msg{$RESP_TYPE_COMMENTARY | $COM_PRINTER_ERROR} =
	["the printer", "PRINTER", "PRINTING_YOUR_PRINTJOB", "PRN_ERROR"];
$msg{$RESP_TYPE_COMMENTARY | $COM_PRINTER_STATUS} =
	["the printer", "PRINTER", "PRINTING_YOUR_PRINTJOB", "PRN_STATUS"];
$msg{$RESP_TYPE_COMMENTARY | $COM_STALL} =
	["the printer", "PRINTER", "PRINTING_YOUR_PRINTJOB", "STALL"];
$msg{$RESP_TYPE_COMMENTARY | $COM_EXIT} =
	["the printer", "PRINTER", "PRINTING_YOUR_PRINTJOB", "EXIT"];

# The keys are from ../pprdrv/pprdrv_snmp_messages.c
my %snmp_error;
$snmp_error{"paper low"} = "is running low on paper";
$snmp_error{"out of paper"} = "is out of paper";
$snmp_error{"toner/ink is low"} = "is running low on toner or ink";
$snmp_error{"out of toner/ink"} = "is out of toner or ink";
$snmp_error{"cover open"} = "has its cover open";
$snmp_error{"paper jam"} = "has a paper jam";
$snmp_error{"off line"} = "is off line";
$snmp_error{"service requested"} = "requires service";
$snmp_error{"no paper tray"} = "is missing a paper tray";
$snmp_error{"no output tray"} = "is missing an output tray";
$snmp_error{"toner/ink cartridge missing"} = "is missing a toner or ink cartridge";
$snmp_error{"output tray near full"} = "has a nearly full output tray";
$snmp_error{"output tray full"} = "has a full output tray";
$snmp_error{"input tray empty"} = "has an empty input tray";
$snmp_error{"overdue preventative maintainance"} = "is overdue for preventative maintainance";

my %snmp_status;
$snmp_status{"other(1)"} = "is in an unspecified state";
$snmp_status{"unknown(2)"} = "is in an unknown state";
$snmp_status{"idle(3)"} = "is idle";
$snmp_status{"printing(4)"} = "is otherwise engaged";
$snmp_status{"warmup(5)"} = "is warming up";

my %exit_codes;
$exit_codes{"EXIT_PRINTED"} =
	[ 'has printed a document' ];
$exit_codes{"EXIT_PRNERR"} =
	[ 'has failed to print due to a communications error' ];
$exit_codes{"EXIT_PRNERR_NORETRY"} =
	[ 'has something badly wrong with it' ];
$exit_codes{"EXIT_JOBERR"} =
	[ 'has detected an error in the current job' ];
$exit_codes{"EXIT_SIGNAL"}=
	[ 'has stopt printing because somebody killed the interface program' ];
$exit_codes{"EXIT_ENGAGED"} =
	[ 'is otherwise engaged or off line' ];
$exit_codes{"EXIT_STARVED"} =
	[ 'is not printing due to an insufficiency of system resources' ];
$exit_codes{"EXIT_PRNERR_NORETRY_ACCESS_DENIED"} =
	["will not let PPR connect"];
$exit_codes{"EXIT_PRNERR_NOT_RESPONDING"} =
	["is not responding"];
$exit_codes{"EXIT_PRNERR_NORETRY_BAD_SETTINGS"} =
	["is not working because the interface settings are incorrect"];
$exit_codes{"EXIT_PRNERR_NO_SUCH_ADDRESS"} =
	["is not working because address lookup fails"];
$exit_codes{"EXIT_PRNERR_NORETRY_NO_SUCH_ADDRESS"} =
	["is not working because its address does not exist"];
$exit_codes{"EXIT_INCAPABLE"} = 
	[ 'is not capable of printing the current job' ];

# Given the responder arguments, produce a list of audio files
# to be played.
sub speach_ppr_response
	{
	my $args = shift;

	# Start to build a list of words and phrases to be played.
	my @playlist;

	# Start with the message template.
	if(defined($message = $msg{$args->{response_code}}))
		{
		push(@playlist, @$message);
		}
	else
		{
		push(@playlist, "?");	# error sound
		}

	# If there is a full jobid, create a playlist for it
	my @job_playlist = ();
	if(defined $args->{job})
		{
		$args->{job} =~ /^(.+)-(\d+)\.(\d)$/ || die;
		my($destination, $id, $subid) = ($1, $2, $3);
		speach_spellout(\@job_playlist, $destination);
		speach_spellout(\@job_playlist, $id);		# say the digits
		#speach_number(\@playlist, $id);			# or pronounce it in high style
		if($subid != 0)
			{
			push(@job_playlist, "part");
			speach_number(\@job_playlist, $subid);
			}
		}

	# Replace all-upper-case words in the message with the results of calling 
	# special routines.
	for($x=0; $x <= $#playlist; $x++)
		{
		if($playlist[$x] =~ /^([A-Z_]+)$/)		# if special word,
			{
			my $key = $1;
			$key =~ tr/[A-Z]/[a-z]/;
			@new = ();
			if($key eq "your_printjob")
				{
				if(scalar @job_playlist > 0)
					{
					push(@new, 'your print job', @job_playlist);
					}
				else
					{
					push(@new, "your new print job for");
					speach_spellout(\@new, $args->{destination});
					}
				}
			elsif($key eq "printing_your_printjob")
				{
				push(@new, 'which is printing your print job', @job_playlist);
				}
			elsif($key eq "pages")
				{
				if(defined $args->{pages})
					{
					push(@new, "(pause)");
					push(@new, "the document is");
			
					if($args->{pages} == 1)
						{
						push(@new, "one page long");
						}
					else
						{
						speach_number(\@new, $args->{pages});
						push(@new, "pages long");
						}
					}
				}
			elsif($key eq "when")
				{
				defined($args->{time}) || die;
				my $elapsed_seconds = (time() - $args->{time});
				if($elapsed_seconds > 0)
					{
					push(@new, "which you submitted");
					speach_time_interval(\@new, $elapsed_seconds);
					push(@new, "ago");
					}
				}
			elsif($key eq "prn_error")
				{
				my $error = $args->{commentary_cooked};
				my $error_snippet = $snmp_error{$error};
				if(defined $error_snippet)
					{
					push(@new, $error_snippet);
					}
				else
					{
					speach_spellout(\@new, $error);
					}
				}
			elsif($key eq "prn_status")
				{
				my $status = $args->{commentary_cooked};
				my $status_snippet = $snmp_status{$status};
				if(defined $status)
					{
					push(@new, $status_snippet);
					}
				else
					{
					speach_spellout(\@new, $status);
					}
				}
			elsif($key eq "stall")
				{
				my $cooked = $args->{commentary_cooked};
				my $duration = $args->{commentary_duration};
				if($cooked eq "STALLED")
					{
					push(@new, 'has been stalled for');
					}
				elsif($cooked eq "UNSTALLED")
					{
					push(@new, 'is no longer stalled');
					}
				else
					{
					die;
					}
				}
			elsif($key eq "exit")
				{
				my $error = $args->{commentary_raw1};
				my $error_snippet = $exit_codes{$error};
				if(defined $error_snippet)
					{
					push(@new, $error_snippet);
					}
				else
					{
					speach_spellout(\@new, $error);
					}
				}
			else		# other special word
				{
				speach_spellout(\@new, $args->{$key});
				}
			splice(@playlist, $x, 1, @new);
			}
		}

	return @playlist;
	}

# Suck the name=value pairs that are our arguments into a hash.
print join(" ", @ARGV), "\n";
my $args = responder_argv(@ARGV);

# Parse the options parameter which is a series of name=value pairs.
foreach my $option (split(/[\s]+/, $args->{responder_options}))
	{
	my($name, $value) = split(/=/, $option);
	if($name eq "voice")
		{
		speach_set_voice($value);
		}
	}

# Get a list of the sound clips to play.
my @playlist = speach_ppr_response($args, $OPTION_SILLY_SOUNDS);
print join(' ', @playlist), "\n";

# Play them
speach_play_many_local($args->{responder_address}, @playlist);

exit(0);
