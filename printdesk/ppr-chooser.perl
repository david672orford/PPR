#! /usr/bin/perl -w
#
# mouse:~ppr/src/printdesk/ppr-chooser.perl
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
# Last modified 5 April 2003.
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
		# "name:type@zone".  If the user pressed "Cancel", then the
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

