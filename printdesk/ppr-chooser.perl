#! /usr/bin/perl -w
#
# mouse:~chappell/printing/printdesk/ppr-chooser.perl
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
# Last modified 7 February 2000.
#

=head1 NAME

ppr-chooser

=head1 SYNOPSIS

B<ppr-chooser>

=head1 DESCRIPTION

This program displays an AppleTalk entity selection dialog.		 When the user
chooses an AppleTalk name and presses the "OK" button, the three part
AppleTalk entity name is printed on stdout.		 If the user chooses "Cancel"
then nothing is printed on stdout.

=cut

use strict;
use Tk;
use lib "?";
use PrintDesk;
use PrintDesk::ATchooser;

# Create a top level GUI window.
my $main_window = new MainWindow;

# This application has no use for its main window and we can't convienently
# put the chooser it it, so we will just hide it.
$main_window->withdraw();

# The action routine will store the value it wants the application to exit
# with in this variable.
$main::retcode = 255;

# This widget is designed to be called in response to a button press.
# this will be simulated by $main->after().		 This is the action routine.
sub choose
		{
		my $main_window = shift;

		# Create an instance of the AppleTalk chooser widget.
		my $chooser = new PrintDesk::ATchooser($main_window);

		# What type of device are we looking for?  The default
		# is "LaserWriter".
		#$chooser->set_type("LaserShared");

		# Get the choice from the widget.
		my $choice = $chooser->Show();

		# The return value will be an AppleTalk address in the form
		# "name:type@zone".		 If the user pressed "Cancel", then the
		# value will be undefined.
		if(defined($choice))
				{
				print "$choice\n";
				$main::retcode = 0;
				}
		else
				{
				print "\n";
				$main::retcode = 1;
				}

		$main_window->destroy();
		}

$main_window->after(1, [\&choose, $main_window]);

MainLoop;

exit $main::retcode;

