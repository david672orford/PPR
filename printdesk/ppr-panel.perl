#! @PERL_PATH@ -w
#
# mouse:~ppr/src/printdesk/ppr-panel.perl
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
# Last modified 17 January 2005.
#

use strict;
use POSIX qw(locale_h);
use Locale::gettext;
use Tk;

use lib "@PERL_LIBDIR@";
use PPR;
use PrintDesk;
use PrintDesk::GENfolder;
use PrintDesk::PPRlistqueue;
use PrintDesk::GENjobbuttons;
use PrintDesk::PPRprnstatus;
use PrintDesk::GENprnbuttons;
use PrintDesk::PPRprintdialog;
use PPR::PPOP;

my $main_window;

#==========================================================================
# This allows us to make subroutines which will use the main window
# if they aren't passed a pointer to it but will create a new toplevel
# if they are passed a pointer to a main window.
#==========================================================================
sub my_window
	{
	my $main = shift;
	my $title = shift;
	my $toplevel;

	if(defined $main)
		{
		$toplevel = $main->Toplevel();
		}
	else
		{
		$toplevel = $main_window;
		}

	$toplevel->title($title);

	return $toplevel;
	}

#==========================================================================
# This is called when the user double-clicks on a printer
# or group icon.
#==========================================================================
sub do_queue
	{
	my($main, $queuename) = @_;

	#print "do_queue(\"$main\", \"$queuename\")\n";

	my $w = my_window($main, "Queue \"$queuename\"");

	# Create a menu bar at the top.
	my $menuframe = $w->Frame(
			)->pack(-side => 'top', -fill => 'x');

	# Create the Queue menu.
	{
	my $menu_queue = $menuframe->Menubutton(
			-text => 'Queue',
			-tearoff => 0
			)->pack(-side => 'left');
	$menu_queue->command(
			-label => defined($main) ? "Close" : "Exit",
			-command => sub { $w->destroy() });
	}

	# Create the Printer menu
	{
	my $menu_printer = $menuframe->Menubutton(
			-text => 'Printer',
			-tearoff => 0
			)->pack(-side => 'left');
	my $ppr_control = new PPR::PPOP($queuename);
	foreach my $printer ($ppr_control->list_members())
			{
			$menu_printer->command(-label => "Status of $printer",
							-command => sub{do_prnstatus($w, $printer)});
			}
	$ppr_control->destroy();
	}

	# Create queue listing frame, divider and button frame.
	$w->Frame(
			-height => 1,
			-bg => 'black'
			)->pack(-side => 'top', -fill => 'x');
	my $queue_frame = $w->Frame(
			)->pack(-side, 'top', -fill, 'both', -expand, 1);
	$w->Frame(
			-height => 1,
			-bg => 'black'
			)->pack(-side => 'top', -fill => 'x');
	my $job_buttons_frame = $w->Frame(
			)->pack(-side => 'top' => -fill, 'x');

	# Put a queue lister in the top frame.
	my $queue = new PrintDesk::PPRlistqueue($queue_frame, $queuename);

	# Put job control buttons in the bottom frame.
	my $job_buttons = new PrintDesk::GENjobbuttons($job_buttons_frame, $queue);
	$job_buttons->AddButton("Close", sub { $w->destroy() });

	# Make the queue appear.
	$queue->Show();
	$job_buttons->Show();

	# When this window is closed, destroy the queue display
	# and button box widgets.
	$w->OnDestroy(sub
			{
			$queue->destroy();
			$job_buttons->destroy()
			}
			);
	}

#==========================================================================
# This is called to display a printer's status window.
#==========================================================================
sub do_prnstatus
	{
	my($main, $printer) = @_;

	#print "do_prnstatus(\"$main\", \"$printer\")\n";

	my $toplevel = my_window($main, "Status of printer \"$printer\"");

	# Printer status frames:
	my $printer_frame_left = $toplevel->Frame()->pack(-side, 'left', -fill, 'y');
	my $printer_frame_right = $toplevel->Frame()->pack(-side, 'right', -fill, 'y');

	# Put printer status in its frame.
	my $printer_status = new PrintDesk::PPRprnstatus($printer_frame_left, $printer);

	# Put printer buttons into their frame.
	my $prn_buttons = new PrintDesk::GENprnbuttons($printer_frame_right, $printer_status);
	$prn_buttons->AddButton('Close', sub{ $toplevel->destroy() });

	$printer_status->Show();
	$prn_buttons->Show();

	$toplevel->OnDestroy(sub {
		$printer_status->destroy();
		$prn_buttons->destroy();
		}
		);
	}

#==========================================================================
# This is called when the user clicks on a printer or group icon with
# the right mouse button.  It pops up a menu.
#==========================================================================
my $previous_menu = undef;
sub do_menu
	{
	my($parent, $queuename, $desttype) = @_;

	#print "do_menu(\"$parent\", \"$queuename\", \"$desttype\")\n";

	# Destroy the previous popup menu
	if(defined($previous_menu))
			{
			$previous_menu->destroy();
			}

	# Create the menu
	my $menu = $parent->Menu(-tearoff, 0);
	$menu->command(
			-label => "Queue",
			-command => sub{do_queue($parent, $queuename)});

	# Add one item for each member
	if($desttype eq 'printer')
		{
		$menu->command(-label => "Status of $queuename",
				-command => sub{do_prnstatus($parent, $queuename)});
		}
	else
		{
		my $ppr_control = new PPR::PPOP($queuename);
		foreach my $printer ($ppr_control->list_members())
			{
			$menu->command(-label => "Status of $printer",
					-command => sub{do_prnstatus($parent, $printer)});
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
	my $dest = shift;

	my $w = my_window($main, "Print");

	my $dialog = new PrintDesk::PPRprintdialog($w);

	my @args = $dialog->Show($dest);

	if(scalar @args > 0)
		{
		foreach my $file (@_)
			{
			print join(' ', "$PPR::PPR_PATH", @args, $file), "\n";
			system("$PPR::PPR_PATH", @args, $file) && die;
			}
		}

	$dialog->destroy();
	}

#==========================================================================
# Create the PPR control panel.
#==========================================================================

sub do_panel
	{
	my $main = shift;

	my $w = my_window($main, "PPR Queues");

	# Create a menu bar at the top.
	my $menuframe = $w->Frame();
	$menuframe->pack(-fill => 'x');

	# Create the File menu.
	my $menu_file = $menuframe->Menubutton(-text => 'File', -tearoff, 0);
	$menu_file->pack(-side => 'left');
	$menu_file->command(-label => 'Exit', -command => sub { $w->destroy() });

	$w->Frame(
		-height => 1,
		-bg => 'black'
		)->pack(-side => 'top', -fill => 'x');

	# Create an instance of the object which displays a list
	# of printers and groups.
	my $folder = new PrintDesk::GENfolder($w, \&do_queue, \&do_menu);

	# Make the folder appear.
	$folder->Show();
	}

#==========================================================================
# Main
#==========================================================================

setlocale(&LC_ALL, "");
bindtextdomain($PPR::PACKAGE, $PPR::LOCALEDIR);
textdomain($PPR::PACKAGE);

$main_window = MainWindow->new();
#$main_window->bisque;

my $locale = setlocale(&LC_CTYPE);
print "\$locale = $locale\n";
if($locale =~ /^(.+)\.(.+)$/)
	{
	my $charset = $2;
	$charset =~ tr/[A-Z]/[a-z]/;
	$charset = "iso10646-1" if($charset eq "utf-8");
	$main_window->optionAdd("*font" => "-*-helvetica-medium-r-normal--*-120-*-*-p-*-$charset");
	}

my $opt_dest = undef;
while(my $arg = shift @ARGV)
	{
	if($arg eq "-d" || $arg eq "--dest")
		{
		$opt_dest = shift @ARGV;
		}
	elsif($arg !~ /^-/ || $arg eq "-")
		{
		unshift(@ARGV, $arg);
		last;
		}
	else
		{
		die;
		}
	}

if(scalar @ARGV > 0)
	{
	do_drop(undef, $opt_dest, @ARGV);
	}
elsif(defined $opt_dest)
	{
	do_queue(undef, $opt_dest, undef);
	}
else
	{
	do_panel();
	}

# Start the GUI event loop.
MainLoop;

# We should get here after $main->destroy() terminates
# the GUI event handler.
exit 0;
