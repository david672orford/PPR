#=============================================================================
# mouse:~ppr/src/printdesk/GENjobbuttons.pm
# Copyright 1995--1999, Trinity College Computing Center.
# Written by David Chappell.
#
# Last modified 5 January 1999.
#=============================================================================
package PrintDesk::GENjobbuttons;

require 5.003;
require Exporter;
use PrintDesk;
use PrintDesk::BatchDialog;

@ISA = qw(Exporter);
@EXPORT = qw();

sub new
    {
    #print "PrintDesk::GENjobbuttons::new()\n";

    my $self = {};
    bless $self;

    shift;				# junk
    $self->{window} = shift;		# frame or toplevel it build it in
    $self->{listqueue} = shift;		# listqueue object to interact with
    $self->{extra_button_list} = [];	# those added by AddButton()
    $self->{batch_dialog} = new PrintDesk::BatchDialog($self->{window});

    return $self;
    }

#
# This routine takes as its argument a reference to the function which
# should be called on each of the selected jobs.
# The function in question is a member function of $jobcontrol.
#
# The second argument is a one work description of the action.
#
sub do_it
    {
    my $self = shift;
    my $action_function = shift;
    my $action_description = shift;

    my $listqueue = $self->{listqueue};
    my @selected = $listqueue->getSelection();
    my $jobcontrol = $listqueue->get_jobcontrol();
    my $batch_dialog = $self->{batch_dialog};

    #print "\$self = $self, \$action_function = $action_function, \$action_description = $action_description\n";

    if($#selected < 0)
	{
	$batch_dialog->Show("First you must select something to $action_description .", "OK");
	return;
	}

    my $job;
    my $remaining = $#selected + 1;
    foreach $job (@selected)
	{
	#print "Doing $job\n";
	my @output = &$action_function($jobcontrol, $job);

	if($#output >= 0)
	    {
	    unshift(@output, "Can't $action_description $job.");
	    if($remaining > 1)
		{
		if( $batch_dialog->Show(join(' ', @output), "Try Next", "Abort") eq "Abort")
		    { last; }
		}
	    else
		{
		$batch_dialog->Show(join(' ', @output), "-Try Next", "Abort");
		}
	    }
	} continue { $remaining--; }
    }

sub Show
    {
    my $self = shift;

    my $window = $self->{window};
    my $listqueue = $self->{listqueue};
    my $jobcontrol = $listqueue->get_jobcontrol();

    $window->Button(-text, "Cancel Job",
	-command, [sub {$_[0]->do_it($_[1], "cancel")}, $self, \&{ref($jobcontrol) . "::cancel"}])->pack(-side, 'left');

    $window->Button(-text, "Rush Job",
	-command, [sub {$_[0]->do_it($_[1], "rush")}, $self, \&{ref($jobcontrol) . "::rush"}])->pack(-side, 'left');

    $window->Button(-text, "Hold Job",
	-command, [sub {$_[0]->do_it($_[1], "hold")}, $self, \&{ref($jobcontrol) . "::hold"}])->pack(-side, 'left');

    $window->Button(-text, "Release Job",
	-command, [sub {$_[0]->do_it($_[1], "release")}, $self, \&{ref($jobcontrol) . "::release"}])->pack(-side, 'left');

#    $window->Frame()->pack(-side, 'left', -expand, 1);

    my $button;
    foreach $button (@{$self->{extra_button_list}})
	{
	$button->pack(-side, 'right');
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

    my $window = $self->{window};
    my $extra_button_list = $self->{extra_button_list};

    push(@$extra_button_list, $window->Button(-text, $text, -command, $command));
    }

#
# Destroy this window
#
sub destroy
    {
    my $self = shift;
    #print "PrintDesk::GENjobbuttons::destroy()\n";
    }

1;
