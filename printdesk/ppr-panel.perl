#! /usr/bin/perl -w
#
# mouse:~ppr/src/ppr/printdesk/ppr-panel.perl
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
# Last modified 17 March 2003.
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
sub do_drop
    {
    my $main = shift;

    my $dialog = new PrintDesk::PPRprintdialog($main);

    my(@args) = $dialog->Show();

    if(scalar @args > 0)
	{
	foreach my $file (@_)
	    {
	    system("$HOMEDIR/bin/ppr", @args, $file) && die;
	    }
	}

    $dialog->destroy();
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
