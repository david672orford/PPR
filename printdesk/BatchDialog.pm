#================================================================
# mouse:~ppr/src/printdesk/BatchDialog.pm
# Copyright 1995--1999, Trinity College Computing Center.
# Written by David Chappell.
#
# Last modified 5 January 1999.
#================================================================

package PrintDesk::BatchDialog;

require 5.003;
#require Exporter;

use Carp;
use PrintDesk;

#@ISA = qw(Exporter);
#@EXPORT = qw();

#
# Constructor.
#
# $dialog = new PrintDesk::BatchDialog($parent, -title, "Can't perform operation");
#
sub new
    {
    shift;
    my $parent = shift;
    my @options = @_;

    #print "PrintDesk::BatchDialog::new()\n";

    defined($parent) || croak "Missing parent parameter";

    my $self = {};
    bless $self;

    $self->{title} = "Error!";
    $self->{width} = $default_width;
    $self->{font} = $default_font;

    my $x;
    for($x=0; $x < $#options; $x += 2)
	{
	if($options[$x] eq "title")
	    { $self->{title} = $options[$x + 1]; }
	elsif($options[$x] eq "width")
	    { $self->{width} = $options[$x + 1]; }
	elsif($options[$x] eq "font")
	    { $self->{font} = $options[$x + 1]; }
	else
	    { croak "Unrecognized option $options[$x]"; }
	}

    # Create a toplevel window and assign its title
    # and icon name.
    my $window = $parent->Toplevel(-class, 'Dialog');
    $window->title($self->{title});
    $window->iconname($self->{title});
    $window->withdraw();		# hide until needed

    # Create a message widget to hold the wrapped message text.
    $self->{message} = $window->Message(-width, $self->{width},
	-font, $self->{font})->pack(-side, 'top');

    # Create a frame to hold the buttons.
    my $button_frame = $self->{button_frame} = $window->Frame();
    $button_frame->pack(-side, 'right');

    #print "New PrintDesk::BatchDialog, parent = $parent, window = $window\n";

    $self->{window} = $window;

    return $self;
    }

#
# $button = $dialog->Show("This is the message", "cancel", "continue");
#
sub Show
    {
    my $self = shift;
    my $text = shift;
    my @buttons = @_;

    my $window = $self->{window};
    my $button_frame = $self->{button_frame};
    my $done = "";
    my @button_widgets = ();

    # Put the current message into the message widget.
    $self->{message}->configure(-text, $text);

    # Create an OK button which whill set $done to true when it is pressed.
    my $button;
    foreach $button (@buttons)
	{
	my $button_widget;
	if(substr($button, 0, 1) eq "-")
	    {
	    $button_widget = $button_frame->Button(-text, substr($button, 1),
		-state, 'disabled',
		-command, [sub{$done = $_[0];}, $button]);
	    }
	else
	    {
	    $button_widget = $button_frame->Button(-text, $button,
		-command, [sub{$done = $_[0];}, $button]);
	    }
	$button_widget->pack(-side, 'left');
	push(@button_widgets, $button_widget);
	}

    # Display the window.  We must flush display updates
    # first, otherwise the window will get displayed before it is redrawn.
    $window->idletasks();
    $window->deiconify();

    # Ring the display bell:
    $window->bell();

    # Save the old focus and grab by asking for references
    # to anonymous subroutines to restore them and then
    # and move the focus to the directory selection window.
    my $old_focus = $window->focusSave();
    my $old_grab = $window->grabSave();
    $window->grab();
    $window->focus();

    # Wait for the dialog to be complete
    $window->waitVariable(\$done);
    $window->grabRelease();
    $window->withdraw();
    &$old_focus;
    &$old_grab;

    # Unpack the buttons
    foreach $button (@button_widgets)
	{
	$button->destroy();
	}

    return $done;
    }

sub destroy
    {
    my $self = shift;
    #print "PrintDesk::BatchDialog::destroy()\n";
    }

1;

__END__
=head1 NAME
PrintDesk::BatchDialog

=head1 SYNOPSIS

=head1 DESCRIPTION

=cut
