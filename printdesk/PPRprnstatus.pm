#
# mouse:~ppr/src/printdesk/PPRprnstatus.pm
# Copyright 1995--2003, Trinity College Computing Center.
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
# Last modified 31 October 2003.
#

package PrintDesk::PPRprnstatus;

use PrintDesk;
use PPR::PPOP;
use PrintDesk::PPRstateupdate;
use Tk::ROText;

require 5.003;
require Exporter;

@ISA = qw(Exporter);
@EXPORT = qw();

#
# Create a new PPRprnstatus object.
#
sub new
	{
	#print "PrintDesk::PPRprnstatus::new()\n";

	my $self = {};
	bless $self;

	shift;								  # junk
	$self->{window} = shift;			  # frame or toplevel to build it in
	$self->{printer} = shift;			  # printer to display

	#print "Creating a new PPRprnstatus object for queue \"$self->{printer}\"\n";

	# Create a PPRcontrol object which we will use to
	# control the printer.
	$self->{prncontrol} = new PPR::PPOP($self->{printer});

	return $self;
	}

#
# Destroy this PPRprnstatus object.
#
sub destroy
	{
	my $self = shift;
	#print "PrintDesk::PPRprnstatus::destroy()\n";
	$self->{prncontrol}->destroy();
	}

#
# Return the job control object for use by the job control
# button box.
#
sub get_prncontrol
	{
	my $self = shift;
	return $self->{prncontrol};
	}

#
# Return the name of the printer.
#
sub get_printer
	{
	my $self = shift;
	return $self->{printer};
	}

#
# Create a small frame containing labeled text.
#
sub LabeledLabel
	{
	my($parent, $description, $text, $width) = @_;

	my $frame = $parent->Frame();
	my $label1 = $frame->Label(-text, $description);
	my $label2 = $frame->Label(-text, $text, -width, $width, -anchor, 'w');
	$label1->pack(-side, 'left');
	$label2->pack(-side, 'left');

	return $frame;
	}

#
# Create a small frame which displays labeled variable contents.
#
sub LabeledVariable
	{
	my($parent, $description, $text, $width) = @_;

	my $frame = $parent->Frame();
	$frame->Label(
		-text, $description
		)->pack(-side => 'left');
	my $label2 = $frame->Label(
		-relief => 'groove',
		-borderwidth => 2,
		-textvariable => $text,
		-width => $width,
		-anchor => 'w',
		-background => 'white'
		)->pack(-side => 'left');

	return $frame;
	}

#
# This public routine should be called when it is time
# to display the listbox.
#
sub Show
  {
  my $self = shift;
  my $w = $self->{window};

  my $prncontrol = $self->{prncontrol};

  my $inner_window = $w->Frame();
  $inner_window->pack(-side, 'top', -padx, 5, -pady, 5, -fill, 'both', -expand, 1);

  # The printer name and comment:
  my $name_window = $inner_window->Frame();
  $name_window->pack(-side, 'top', -anchor, 'w');
  LabeledLabel($name_window, "Name:", $self->{printer}, 16)->
		pack(-side, 'left');
  LabeledLabel($name_window, "Comment:", $prncontrol->get_comment(), 40)->
		pack(-side, 'left');
  $inner_window->Frame(-height, 5)->pack(-side, 'top');

  # A number of labeled variables grouped together.
  my $status_frame = $inner_window->Frame();
  $status_frame->pack(-side, 'top', -anchor, 'w');
  LabeledVariable($status_frame, "Status:", \$self->{status}, 30)->
		pack(-side, 'left');
  LabeledVariable($status_frame, "Retry:", \$self->{retry}, 4)->
		pack(-side, 'left');
  LabeledVariable($status_frame, "Countdown:", \$self->{countdown}, 4)->
		pack(-side, 'left');
  $inner_window->Frame(-height, 5)->pack(-side, 'top');

  # The last message from the printer:
  LabeledVariable($inner_window, "Last Printer Message:", \$self->{message}, 50)->
		pack(-side, 'top', -anchor, 'w');
  $inner_window->Frame(-height, 5)->pack(-side, 'top');

  # Progress on current job:
  my $progress_frame = $inner_window->Frame();
  $progress_frame->pack(-side, 'top', -anchor, 'w');
  LabeledVariable($progress_frame, "Percent sent:", \$self->{percent_sent}, 4)->
		pack(-side, 'left');
  LabeledVariable($progress_frame, "Pages started:", \$self->{pages_started}, 4)->
		pack(-side, 'left');
  LabeledVariable($progress_frame, "Pages completed:", \$self->{pages_completed}, 4) ->
		pack(-side, 'left');
  $inner_window->Frame(-height, 5)->pack(-side, 'top');

  # A scrolling text box for printer alerts:
  my $alerts_frame = $inner_window->Frame(
		)->pack(-side => 'top', -anchor => 'w');
  $alerts_frame->Label(
		-text => "Alert Messages:"
		)->pack(-side => 'top', -anchor => 'w');
  my $alerts_text_frame = $alerts_frame->Frame(
		)->pack(-fill => 'both', -expand => 1);
  my $scrollbar = $alerts_text_frame->Scrollbar(
		)->pack(-side => 'right', -fill => 'y');
  my $alerts_text = $alerts_text_frame->ROText(
		-width => 80,
		-height => 7,
		-setgrid => 1,
		-yscrollcommand => [$scrollbar, 'set'],
		-background => 'white'
		)->pack(-side => 'left', -fill => 'both', -expand => 1);
  $scrollbar->configure(-command, [$alerts_text, 'yview']);
  $self->{alerts_text} = $alerts_text;

  # Fill in the current status:
  {
  my @result_rows = $prncontrol->get_pstatus();
  my @result_row1 = @{shift @result_rows};
  shift @result_row1;	# printer name
  ($self->{status}, $self->{job}, $self->{retry}, $self->{countdown}, $self->{message}) = @result_row1;
  # Temporary hack
  $self->{status} = "$self->{status} $self->{job}";
  }

  # If the printer is in a state that involves a countdown, arang for
  # an update every second.
  if($self->{countdown} > 0) { $self->{tick} = $w->repeat(1000, [\&do_tick, $self]) }

  # Refresh the alerts window.
  $self->update_alerts();

  # Create a state updater process to keep the printer
  # status current.
  my $updater = new PrintDesk::PPRstateupdate($w, $self->{printer});
  $updater->register('pstatus', $self, \&pstatus);
  $updater->register('pmessage', $self, \&pmessage);
  $updater->register('pbytes', $self, \&pbytes);
  $updater->register('ppages', $self, \&ppages);
  $updater->register('pfpages', $self, \&pfpages);
  $updater->register('pexit', $self, \&pexit);
  }

#
# The countdown callback
#
sub do_tick
	{
	my $self = shift;

	if($self->{countdown} > 0)
		{ $self->{countdown}-- }
	else
		{
		$self->{tick}->cancel();
		undef $self->{tick};
		}
	}

#
# Update the alerts text box.
#
sub update_alerts
	{
	my $self = shift;

	my $alerts_text = $self->{alerts_text};

	$alerts_text->delete('1.0', 'end');

	$alerts_text->insert('end', $self->{prncontrol}->get_alerts());

	$alerts_text->see('end');
	}

#
# Callback function for pprdrv exit
#
sub pexit
	{
	my $self = shift;
	$self->{percent_sent} = "";
	$self->{pages_started} = "";
	$self->{pages_completed} = "";
	}

#
# Callback function for printer status
#
sub pstatus
	{
	my($self, $printer, $status, $retry, $countdown) = @_;
	($self->{status}, $self->{retry}, $self->{countdown}) = ($status, $retry, $countdown);

	# If there is a retry countdown, set a timer to tick it off.
	if($countdown > 0 && ! defined($self->{tick}))
		{
		$self->{tick} = $self->{window}->repeat(1000, [\&do_tick, $self]);
		}

	# If the state is fault, update the alert window.
	if($status =~ /^fault/)
		{
		$self->update_alerts();
		}
	}

#
# Callback function for the printer message
#
sub pmessage
	{
	my($self, $printer, $message) = @_;
	$self->{message} = $message;
	}

#
# Callback routines for the various progress fields
#
sub pbytes
	{
	my($self, $printer, $sent, $total) = @_;
	$self->{percent_sent} = int($sent/$total * 100 + 0.5);
	}
sub ppages
	{
	my($self, $printer, $pages) = @_;
	$self->{pages_started} = $pages;
	}
sub pfpages
	{
	my($self, $printer, $pages) = @_;
	$self->{pages_completed} = $pages;
	}

1;

__END__
=head1 NAME
PrintDesk::PPRprnstatus

=head1 SYNOPSIS

$p = new PrintDesk::PPRprnstatus($window, $printer);
$p->Show();
$p->destroy();

=head1 DESCRIPTION

This widget displays the status of a PPR printer.

=cut

