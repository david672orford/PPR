#
# mouse:~ppr/src/libscript/speach_commentary.pl
# Copyright 1995--2001, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appears in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is"
# without express or implied warranty.
#
# Last modified 4 May 2001.
#

#
# This file contains a subroutine speach_printer_commentary() which returns
# a list of sound files suitable for passing to play_many().
#

#
# To extract the message strings from this file use:
# sed -n -e "s/^.*\'(.*\)'.*$/\1/p" speach_commentary.pl
#

#
# The type of commentary (the possible values of our $CODE argument.
# The official definitions of these values are in ~ppr/src/pprdrv/pprdrv.h.
#
require 'commentary.ph';

#my $COM_PRINTER_ERRORS = 1;			# %%[ PrinterError: xxxxx ]%%
#my $COM_PRINTER_STATUS = 2;			# %%[ status: xxxxx ] %%%
#my $COM_STALL = 4;						# printer bogging down
#my $COM_EXIT = 8;						# why did it exit?

#===================================================================
# Various classes of silly sounds.	We define them here so that you
# can see all of them at once and because we use some of them
# for whole classes of errors and don't want to duplicate them.
#===================================================================

# Job was printed.
my $SILLY_PRINTED = [
		30 => "*trumpet1",
		30 => "*trumpet2",
		12 => "*flyby"
		];

# Printer faults, two different degrees.
my $SILLY_FAULT = [
		20 => "*ominousgong",
		20 => "*mystery",
		30 => "*sonar",
		20 => "*clank"
		];
my $SILLY_FAULT_NO_RETRY = [
		100 => "*siren"
		];

# Job was deliberately canceled while printing.
my $SILLY_CANCELED = [
		10 => "*biglaser",
		10 => "*blatblatblat",
		10 => "*eraser",
		10 => "*machinegun",
		25 => "*raygun",
		05 => "*riffleshots"
		];

# May be stalled, has been stalled, etc.
my $SILLY_STALLED = [
		15 => "*mystery",
		40 => "*ominousgong"
		];

# May be stalled proved unfounded.
my $SILLY_WASNTSTALLED = [
		50 => "*flyby",
		25 => "*tinklemusic"
		];

# Played before and after announcement that the printer is no
# longer stalled.
my $SILLY_NOLONGERSTALLED =
		[
		25 => "*carstart",
		25 => "*flyby"
		];
my $SILLY_NOLONGERSTALLED2 =
		[
		25 => "*tinklemusic",
		75 => "*flyby"
		];

# Off line or otherwise engaged:
my $SILLY_BLOCKED = [
		70 => "*bwabwa"
		];

# Someone must go burp the printer.
my $SILLY_ATTENTION = [
		25 => "*policewhistle.au",
		25 => "*alarm",
		25 => "*foghorn",
		25 => "*computer"
		];

# Paper Jam
my $SILLY_JAM = [
		75 => "*crash",
		25 => "*alarm"
		];

# Something wrong with the job, probably a PostScript error.
my $SILLY_JOBERROR = [
		50 => "*glass",
		25 => "*bounce",
		25 => "*bwabwa"
		];

# This sound is played when we say that the printer has been judged
# incapable of printing the job.
my $SILLY_INCAPABLE = [
		75 => "*bounce",
		25 => "*bwabwa"
		];

#==============================================================
# The events and their sound files, grouped by $CODE value.
#
# Each array has 3 elements:
# 0		silly sound set to play at start
# 1		phrase for body of message
# 2		silly sound set to play at end
#==============================================================

#
# $CODE == $COM_PRINTER_ERRORS
#		or
# $CODE == $COM_PRINTER_STATUS
#
# These codes are used when the printer sends
#		%%[ status: PrinterError: xxxxx ]%%
#				or
#		%%[ status: xxxxx ]%%"
# respecively.
#
my %events_status;
$events_status{"resetting printer"}		= [ undef, 'is resetting' ];
$events_status{"initializing"}			= [ undef, 'is initializing' ];
$events_status{"warming up"}			= [ undef, 'is warming up' ];
$events_status{"idle"}					= [ undef, 'is idle' ];
$events_status{"busy"}					= [ undef, 'is otherwise engaged' ];
$events_status{"printing test page"}	= [ undef, 'is printing a test page' ];

$events_status{"off line"}				= [ $SILLY_ATTENTION, 'is off line' ];
$events_status{"paper low"}				= [ $SILLY_ATTENTION, 'is running low on paper' ];
$events_status{"out of paper"}			= [ $SILLY_ATTENTION, 'is out of paper' ];
$events_status{"no paper tray"}			= [ $SILLY_ATTENTION, 'is missing a paper tray' ];
$events_status{"paper jam"}				= [ $SILLY_JAM, 'has a paper jam' ];
$events_status{"cover open"}			= [ $SILLY_ATTENTION, 'has its cover open' ];
$events_status{"manual feed"}			= [ $SILLY_ATTENTION, 'requires manual feeding' ];
$events_status{"toner/ink is low"}		= [ undef, 'is running low on toner or ink' ];
$events_status{"toner/ink cartridge missing"} = [ undef, 'is missing a toner or ink cartridge' ];

# $CODE == $COM_STALL
# If the printer seems too slow we will receive
# one of the messages below.
my %events_impatience;
$events_impatience{"may be stalled"}			= [ $SILLY_STALLED, 'may be stalled' ];
$events_impatience{"is probably stalled"}		= [ $SILLY_STALLED, 'is probably stalled' ];
$events_impatience{"wasn't stalled"}			= [ $SILLY_WASNTSTALLED, 'is not stalled' ];
$events_impatience{"is no longer stalled"}		= [ $SILLY_NOLONGERSTALLED, 'is no longer stalled', $SILLY_NOLONGERSTALLED2 ];

# $CODE == $COM_EXIT
# Describe the circumstances under which pprdrv exited.
my %events_exit;
$events_exit{"has printed a job"}						= [ $SILLY_PRINTED, 'has printed a document' ];
$events_exit{"printer error"}							= [ $SILLY_FAULT, 'has failed to print due to a communications error' ];
$events_exit{"printer error, no retry"}					= [ $SILLY_FAULT_NO_RETRY, 'has something badly wrong with it' ];
$events_exit{"interface core dump"}						= [ undef, 'has a defective interface program' ];
$events_exit{"job error"}								= [ $SILLY_JOBERROR, 'has detected an error in the current job' ];
$events_exit{"postscript error"}						= [ $SILLY_JOBERROR, 'has detected a postscript error in the current job' ];
$events_exit{"interface program killed"}				= [ undef, 'has stopt printing because somebody killed the interface program' ];
$events_exit{"printing halted"}							= [ $SILLY_CANCELED, 'has stopt printing the current job as ordered' ];
$events_exit{"otherwise engaged or off-line"}			= [ undef, 'is otherwise engaged or off line', $SILLY_BLOCKED ];
$events_exit{"starved for system resources"}			= [ undef, 'is not printing due to an insufficiency of system resources', $SILLY_BLOCKED ];
$events_exit{"incapable of printing this job"}			= [ $SILLY_INCAPABLE, 'is not capable of printing the current job' ];

#
# This routines assembles a playlist of .au files to describe a
# PPR printer commentary message.
#
sub speach_ppr_commentary
{
my ($PRINTER, $CODE, $COOKED, $RAW1, $RAW2, $SEVERITY, $silly_sounds) = @_;
my $event_sound;

# Here is where we store possible silly sounds appropriate for the message.
my $silly_intro_sound = undef;
my $silly_closing_sound = undef;

# Start with an empty playlist.
my @playlist = ();

# Introduce the printer.
push(@playlist, 'the printer');

#
# Convert the printer name to a sound file name.  If there
# is no sound file for this printer, build it out of the
# sound files for the characters of its name.
#
if(speach_sound_available($PRINTER))
	{
	push(@playlist, $PRINTER);
	}
else
	{
	speach_spellout(\@playlist, $PRINTER);
	}

#
# If the event was a printer error, append the sound file
# for that error.
#
# Notice that if there is a cooked version of the message
# we use that, otherwise we use the raw version.  We will probably
# only get a match on the raw one when there is no cooked one if
# we have in the table a raw message which is not translated in
# lw_errors.conf.
#
if($CODE & $COM_PRINTER_ERRORS)
	{
	my $message = $COOKED ne "" ? $COOKED : $RAW1;

	if(defined($event_sound = $events_status{$message}))
		{
		$silly_intro_sound = $event_sound->[0];
		push(@playlist, $event_sound->[1]);
		$silly_closing_sound = $event_sound->[2];
		}
	else
		{
		push(@playlist, 'has reported an unrecognized error');
		}
	}

#
# If it is a status message, look it up in the list of status messages,
# if it is not found there, look it up in the list of printer errors.
# (Many printer status messages are really errors such as "cover open".)
#
# Again notice that we prefer the cooked version but will use the raw
# message.
#
elsif($CODE & $COM_PRINTER_STATUS)
	{
	my $message = $COOKED ne "" ? $COOKED : $RAW1;

	# Status can sometimes be an error message:
	if(defined($event_sound = $events_status{$message}))
		{
		$silly_intro_sound = $event_sound->[0];
		push(@playlist, $event_sound->[1]);
		$silly_closing_sound = $event_sound->[2];
		}
	else
		{
		push(@playlist, 'has reported an unrecognized status');
		}
	}

#
# If the problem is that the printer is stalled, build
# a message.  Some of the messages are ready-recorded,
# others we must build from components.
#
elsif($CODE & $COM_STALL)
	{
	if(defined($event_sound = $events_impatience{$COOKED}))
		{
		$silly_intro_sound = $event_sound->[0];
		push(@playlist, $event_sound->[1]);
		$silly_closing_sound = $event_sound->[2];
		}
	elsif( $COOKED =~ /^has been stalled for ([0-9]+) minutes$/ )
		{
		$silly_intro_sound = $SILLY_STALLED;
		push(@playlist, 'has been stalled for');
		speach_time_interval(\@playlist, ($1 * 60));
		}
	else
		{
		push(@playlist, 'is the subject of an unrecognized stall message');
		}
	}

#
# If it is a pprdrv exit message,
#
elsif($CODE & $COM_EXIT)
	{
	if( defined( $event_sound = $events_exit{$COOKED} ) )
		{
		$silly_intro_sound = $event_sound->[0];
		push(@playlist, $event_sound->[1]);
		$silly_closing_sound = $event_sound->[2];
		}
	else
		{
		push(@playlist, 'is the subject of an invalid exit message');
		}
	}

#
# If none of the above,
#
else
	{
	push(@playlist, 'is named in an invalid event report');
	}

# If an introductory sound was chosen, push it onto the front
# of the play list.
if($silly_sounds)
	{
	if(defined($silly_intro_sound))
		{ unshift(@playlist, $silly_intro_sound); }
	if(defined($silly_closing_sound))
		{ push(@playlist, $silly_closing_sound); }
	}

return @playlist;
}

1;

