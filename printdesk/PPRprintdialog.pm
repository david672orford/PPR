#================================================================
# ~ppr/src/printdesk/PPRprintdialog.pm
# Copyright 1995--1999, Trinity College Computing Center.
# Writen by David Chappell.
#
# Last modified 5 January 1999.
#================================================================

package PrintDesk::PPRprintdialog;
require 5.003;

sub new
    {
    shift;
    my $self = {};
    bless $self;
    $self->{main} = shift;
    return $self;
    }

sub Show
    {
    my $self = shift;

    my $window = $self->{main}->Toplevel();
    $window->title("Print");

    my $right_buttons_frame = $window->Frame();
    $right_buttons_frame->pack(-side, 'right', -anchor, 'n');
    my $print_button = $right_buttons_frame->Button(-text, "Print");
    $print_button->pack(-side, 'top', -fill, 'x');
    my $cancel_button = $right_buttons_frame->Button(-text, "Cancel");
    $cancel_button->pack(-side, 'top', -fill, 'x');


    }

sub destroy
    {

    }

sub printFile
    {
    my $self = shift;

    }

sub get_printHandle
    {
    my $self = shift;

    }

1;

