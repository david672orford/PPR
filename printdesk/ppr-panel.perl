#! /usr/bin/perl -w
#
# mouse:~src/ppr/printdesk/ppr-panel.perl
# Copyright 1995--2000, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is" without
# express or implied warranty.
#
# Last modified 22 September 2000.
#

use strict;
use Tk;

use lib "?";
use PrintDesk;
use PrintDesk::GENfolder;
use PrintDesk::PPRlistqueue;
use PrintDesk::GENjobbuttons;
use PrintDesk::PPRprnstatus;
use PrintDesk::GENprnbuttons;
use PrintDesk::PPRprintdialog;
use PPR::PPOP;

#==========================================================================
# This is called when the user double-clicks on a printer
# or group icon.
#==========================================================================
sub do_queue
    {
    my($main, $queuename, $desttype) = @_;

    #print "do_queue(\"$main\", \"$queuename\", \"$desttype\")\n";

    my $toplevel = $main->Toplevel();
    $toplevel->title("Queue \"$queuename\"");

    # Create queue listing frame, divider and button frame.
    my $queue_frame = $toplevel->Frame()->pack(-side, 'top', -fill, 'both', -expand, 1);
    $toplevel->Frame(-height, 1, -bg, 'black')->pack(-side, 'top', -fill, 'x');
    my $job_buttons_frame = $toplevel->Frame()->pack(-side, 'top', -fill, 'x');

    # Put a queue lister in the top frame.
    my $queue = new PrintDesk::PPRlistqueue($main, $queue_frame, $queuename);

    # Put job control buttons in the next frame.
    my $job_buttons = new PrintDesk::GENjobbuttons($job_buttons_frame, $queue);
    $job_buttons->AddButton("Close", sub { $toplevel->destroy });

    # Make the queue appear.
    $queue->Show();
    $job_buttons->Show();

    # When this window is closed, destroy the queue display
    # and button box widgets.
    $toplevel->OnDestroy(sub { $queue->destroy(); $job_buttons->destroy() });
    }

#==========================================================================
# This is called to display a printer's status window.
#==========================================================================
sub do_prnstatus
    {
    my($main, $printer) = @_;

    #print "do_prnstatus(\"$main\", \"$printer\")\n";

    # Printer status frames:
    my $toplevel = $main->Toplevel();
    $toplevel->title("Status of printer \"$printer\"");
    my $printer_frame_left = $toplevel->Frame()->pack(-side, 'left', -fill, 'y');
    my $printer_frame_right = $toplevel->Frame()->pack(-side, 'right', -fill, 'y');

    # Put printer status in its frame.
    my $printer_status = new PrintDesk::PPRprnstatus($main, $printer_frame_left, $printer);

    # Put printer buttons into their frame.
    my $prn_buttons = new PrintDesk::GENprnbuttons($printer_frame_right, $printer_status);
    $prn_buttons->AddButton('Close', sub{ $toplevel->destroy() });

    $printer_status->Show();
    $prn_buttons->Show();

    $toplevel->OnDestroy( sub{ $printer_status->destroy(); $prn_buttons->destroy() });
    }

#==========================================================================
# This is called when the user clicks on a printer or group icon with
# the right mouse button.  It pops up a menu.
#==========================================================================
my $previous_menu = undef;
sub do_menu
    {
    my($main, $queuename, $desttype) = @_;

    #print "do_menu(\"$main\", \"$queuename\", \"$desttype\")\n";

    # Destroy the previous popup menu
    if(defined($previous_menu)) { $previous_menu->destroy() }

    # Create the menu
    my $menu = $main->Menu(-tearoff, 0);
    $menu->command(-label => "Queue", -command => sub{do_queue($main, $queuename)});

    # Add one item for each member
    if($desttype eq 'printer')
    	{
	$menu->command(-label => "Status of $queuename",
	    -command => sub{do_prnstatus($main, $queuename)});
    	}
    else
    	{
	my $ppr_control = new PPR::PPOP($queuename);
	my $printer;
	foreach $printer ($ppr_control->list_members())
	    {
	    $menu->command(-label => "Status of $printer",
	    	-command => sub{do_prnstatus($main, $printer)});
	    }
	$ppr_control->destroy();
	}

    # Make the menu appear
    $menu->post($menu->pointerxy());

    # Arrange for future destruction.
    $previous_menu = $menu;
    }

#==========================================================================
# Print some files
#==========================================================================

sub doit
    {
    my $main = shift;

    my $dialog = new PrintDesk::PPRprintdialog($main);

    if($dialog->Show(@_))
	{
	$dialog->printFile(@_)
	}

    }


#==========================================================================
# Create the main application window.
#==========================================================================

# Create the top level window for this application.
my $main = new MainWindow;

# Create a menu bar at the top.
my $menuframe = $main->Frame();
$menuframe->pack(-fill => 'x');
my $menu_file = $menuframe->Menubutton(-text => 'File', -tearoff, 0);
$menu_file->pack(-side => 'left');
$menu_file->command(-label => 'Exit', -command => sub { $main->destroy() });

# Create an instance of the object which displays a list
# of printers and groups.
my $folder = new PrintDesk::GENfolder($main, $main, \&do_queue, \&do_menu);

# Make the folder appear.
$folder->Show();

# Start the GUI event loop.
MainLoop;

# We should get here after $main->destroy() terminates
# the GUI event handler.
exit 0;

#==========================================================================
__END__

=head1 NAME

printdesk

=head1 SYNOPSIS

B<printdesk>

=head1 DESCRIPTION

=cut
