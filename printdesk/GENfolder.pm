#
# mouse:~ppr/src/printdesk/GENfolder.pm
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
# Last modified 27 March 2003.
#

=pod

This module creates a folder which lists the available printers.  The caller
provides callback functions for the double click action and the context
menu.

=cut

package PrintDesk::GENfolder;

require 5.003;
use PrintDesk;
use PPR::PPOP;

sub new
    {
    #print "PrintDesk::GENfolder::new()\n";

    my $self = {};
    bless $self;

    shift;
    $self->{window} = shift;
    $self->{double_action} = shift;
    $self->{popup_menu} = shift;

    return $self;
    }

sub Show
    {
    #print "Printdesk::GENfolder::Show()\n";
    my $self = shift;
    my $window = $self->{window};

    my $scrollbar = $window->Scrollbar();
    $scrollbar->pack(-side, 'right', -fill, 'y');

    my $canvas = $window->Canvas(-height, 400, -width, 200, -bg, 'white',
	-yscrollcommand, ['set', $scrollbar]);
    $canvas->pack(-side, 'left', -fill, 'both', -expand, 1);

    $scrollbar->configure(-command, ['yview', $canvas]);

    # Create an object which controls PPR printers:
    my $control = new PPR::PPOP("all");

    my $dest;
    my $x = 10;
    my $y = 25;
    my $double_action = $self->{double_action};
    my $popup_menu = $self->{popup_menu};
    my $bm_printer = $canvas->Bitmap(-file => "$PrintDesk::BITMAPS/printer.xbm");
    my $bm_group = $canvas->Bitmap(-file => "$PrintDesk::BITMAPS/group.xbm");
    foreach $dest ($control->list_destinations())
	{
	my($dest_name, $dest_type) = @$dest[0, 1];
	my $bitmap = $canvas->createImage($x, $y, -anchor, 'w', -image, $dest_type eq "printer" ? $bm_printer : $bm_group);
	$canvas->bind($bitmap, '<Double-1>', sub{&$double_action($self->{window}, $dest_name, $dest_type)});
	$canvas->bind($bitmap, '<3>', sub{&$popup_menu($self->{window}, $dest_name, $dest_type)});
	$canvas->createText($x + 50, $y, -anchor, 'w', -text, $dest_name);
	$y += 50;
	}

    $control->destroy();

    $canvas->configure(-scrollregion, [0, 0, 400, $y]);
    }

sub destroy
    {
    #print "PrintDesk::GENfolder::destroy()\n";
    my $self = shift;
    }

1;

