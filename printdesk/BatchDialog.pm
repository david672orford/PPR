#
# mouse:~ppr/src/printdesk/BatchDialog.pm
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
# Last modified 7 March 2003.
#

=head1 NAME
PrintDesk::BatchDialog

=head1 SYNOPSIS

=head1 DESCRIPTION

The PrintDesk::BatchDialog object is an error dialog box that can be reused. 
The size, font, and title are set when it is created, but its Show() method
may be used at any time to display an error message.

=cut

package PrintDesk::BatchDialog;

require 5.003;
#require Exporter;

use Carp;
use PrintDesk;

#@ISA = qw(Exporter);
#@EXPORT = qw();

=pod

my $dialog = new PrintDesk::BatchDialog($parent, -title, "Can't perform operation");

Create a new error dialog object.

Options:

=over 4

=item -title

=item -width

=item -font

=back

=cut
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
	$window->withdraw();				# hide until needed

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

=pod

my $pressed_button = $dialog->Show("This is the message", "cancel", "continue");

Display the error message in the first parameter and display buttons with
the names in the subsequent parameters.	 Return the name of the button that
the user presses.

=cut
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
