#================================================================
# mouse:~ppr/src/printdesk/GENprnbuttons.pm
# Copyright 1995--1999, Trinity College Computing Center.
# Written by David Chappell.
#
# Last modified 5 January 1999.
#================================================================

package PrintDesk::GENprnbuttons;

require 5.003;
require Exporter;
use PrintDesk;
use PrintDesk::BatchDialog;

@ISA = qw(Exporter);
@EXPORT = qw();

sub new
    {
    #print "PrintDesk::GENprnbuttons::new()\n";

    my $self = {};
    bless $self;

    shift;				# junk
    $self->{window} = shift;		# frame or toplevel it build it in
    $self->{prnstatus} = shift;		# prnstatus object to interact with

    $self->{printer} = $self->{prnstatus}->get_printer();

    $self->{extra_button_list} = [];	# those added by AddButton()
    $self->{batch_dialog} = new PrintDesk::BatchDialog($self->{window});

    $self->{inner_window} = $self->{window}->Frame();
    $self->{inner_window}->pack(-padx, 5, -pady, 5, -fill, 'both', -expand, 1);

    #print STDERR "New PrintDesk::GENprnbuttons object $self\n";
    #print STDERR "window = $self->{window}, listqueue = $self->{listqueue}\n";

    return $self;
    }

sub destroy
    {
    my $self = shift;
    #print "PrintDesk::GENprnbuttons::destroy()\n";
    }

#
# This routine takes as its argument a reference to the function which
# should be called on each of the selected jobs.
# The function in question is a member function of $prncontrol.
#
# The second argument is a one work description of the action.
#
sub do_it
    {
    my $self = shift;
    my $action_function = shift;
    my $action_description = shift;

    my $prnstatus = $self->{prnstatus};
    my $prncontrol = $prnstatus->get_prncontrol();
    my $batch_dialog = $self->{batch_dialog};
    my $printer = $self->{printer};

    #print "\$self = $self, \$action_function = $action_function, \$action_description = $action_description\n";

    my @output = &$action_function($prncontrol);

    if($#output >= 0)
	{
	unshift(@output, "Can't $action_description $printer.");
	$batch_dialog->Show(join(' ', @output), "Dismiss");
	}
    }

sub Show
    {
    my $self = shift;

    my $inner_window = $self->{inner_window};
    my $prnstatus = $self->{prnstatus};
    my $prncontrol = $prnstatus->get_prncontrol();

    $inner_window->Button(-text, "Start",
	-command, [sub {$_[0]->do_it($_[1], "start")}, $self, \&{ref($prncontrol) . "::start"}])->
		pack(-side, 'top', -fill, 'x');

    $inner_window->Button(-text, "Stop",
	-command, [sub {$_[0]->do_it($_[1], "stop")}, $self, \&{ref($prncontrol) . "::stop"}])->
		pack(-side, 'top', -fill, 'x');

    $inner_window->Button(-text, "Halt",
	-command, [sub {$_[0]->do_it($_[1], "halt")}, $self, \&{ref($prncontrol) . "::halt"}])->
		pack(-side, 'top', -fill, 'x');

    my $button;
    foreach $button (@{$self->{extra_button_list}})
	{
	$button->pack(-side, 'bottom', -fill, 'x');
	}
    }

#
# Add a button
#
sub AddButton
    {
    my $self = shift;
    my $text = shift;
    my $command = shift;

    my $inner_window = $self->{inner_window};
    my $extra_button_list = $self->{extra_button_list};

    push(@$extra_button_list, $inner_window->Button(-text, $text, -command, $command));
    }

1;
