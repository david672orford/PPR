#! /usr/bin/perl -w
#
# mouse:~ppr/printdesk/printdialog.perl
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

pprprintdialog - simple command line interface to the PPRprintdialog class

=head1 SYNOPSIS

B<pprprint> I<filename>

=head1 DESCRIPTION

This is a demonstration program for the PPRprintdialog widget.  This program
doesn't work yet.

=cut

use strict;
use Tk;

use lib "?";
use PrintDesk;
use PrintDesk::PPRprintdialog;

my $main = new MainWindow;
$main->withdraw();

sub doit
    {
    my $dialog = new PrintDesk::PPRprintdialog($main);

    if( $dialog->Show() )
	{ $dialog->printFile($ARGV[0]) }

    exit(0);
    }

$main->after(1, [\&doit, $main]);

MainLoop;

