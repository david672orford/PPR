#========================================================================
# mouse:~ppr/src/printdesk/GENfolder.pm
# Copyright 1995--2000, Trinity College Computing Center.
# Written by David Chappell.
#
# This module creates a folder which lists the available
# printers.  The caller provides callback functions for
# the double click action and the context menu.
#
# Last modified 22 September 2000.
#========================================================================

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
    $self->{main} = shift;
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

    my $canvas = $window->Canvas(-height, 400, -width, 400, -bg, 'white',
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
	$canvas->bind($bitmap, '<Double-1>', sub{&$double_action($self->{main}, $dest_name, $dest_type)});
	$canvas->bind($bitmap, '<3>', sub{&$popup_menu($self->{main}, $dest_name, $dest_type)});
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

