#
# mouse:~ppr/src/printdesk/PPRprintdialog.pm
# Copyright 1995--2003, Trinity College Computing Center.
# Writen by David Chappell.
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
# Last modified 28 March 2003.
#

package PrintDesk::PPRprintdialog;

require 5.003;
use PrintDesk;
use PPR::PPOP;
use Tk::LabFrame;

#======================================================================
=item $dialog = new PrintDesk::PPRprintdialog(I<$container>);

=cut
#======================================================================
sub new
    {
    shift;
    my $self = {};
    bless $self;
    $self->{window} = shift;
    $self->{copies} = 1;
    $self->{duplex} = "DuplexNoTumble";
    $self->{inputslot} = "";
    $self->{page_start} = "";
    $self->{page_end} = "";
    return $self;
    }

#======================================================================
=item $dialog->destroy();

=cut
#======================================================================
sub destroy
    {
    my $self = shift;
    $self->{window}->destroy() if(defined $self->{window});
    }

#======================================================================
=item $dialog->Show(I<$default_queue>);

=cut
#======================================================================
sub Show
    {
    my $self = shift;
    $self->{dest} = shift;

    my $w = $self->{window};

    #======================================================================
    # Bottom right frame with [Cancel] and [Print].  Both set a variable
    # and destroy the widget, causing this function to return.
    #======================================================================
    my $br_frame = $w->Frame(
	)->pack(-side, 'bottom', -fill => 'x', -padx => 5, -pady => 5);
    $self->{pressed} = "";
    $br_frame->Button(-text => "Print", -command =>
	sub {
		$self->{pressed} = "Print";
		$self->{window}->destroy();
		}
	)->pack(-side => 'right');
    $br_frame->Button(-text => "Cancel", -command => 
	sub {
		$self->{pressed} = "Cancel";
		$self->{window}->destroy();
		}
	)->pack(-side => 'right');
#    $br_frame->Button(-text => "More Options"
#	)->pack(-side => 'left');

    #======================================================================
    # The top frame contains a queue select widget and a 
    # [Options] button.
    #======================================================================
    my $tl_frame = $w->Frame(
	)->pack(-side => 'top', -fill => 'x', -padx => 5, -pady => 5);
    $tl_frame->Label(
	-text => "Queue:"
	)->pack(-side => 'left');
    my $queue_optionmenu = $tl_frame->Optionmenu(
	-variable => \$self->{dest},
	-textvariable => \$self->{dest_longname},
	-justify => 'left',
	-command => sub { $self->dest_features_load() },
	)->pack(-side => 'left');
#    $tl_frame->Button(-text => 'Options', -command =>
#	sub {
#		}
#	)->pack(-side => 'right');


    #======================================================================
    # Middle frame 1
    #======================================================================
    my $mid_frame_1 = $w->Frame(
	)->pack(-side => 'top', -fill => 'x');

    # ===== Middle frame 1 left: Page range =====
    my $pages_frame = $mid_frame_1->LabFrame(-label => 'Print Range', -labelside => 'top'
	)->pack(-side => 'left', -fill => 'y', -padx => 5, -pady => 5);
    my $pages_which = ($self->{page_start} eq "" && $self->{page_end} eq "") ? 1 : 2;
    my $pages_frame1 = $pages_frame->Frame(
	)->pack(-side => 'top', -anchor => 'w');
    my $pages_rb1 = $pages_frame1->Radiobutton(
	-variable => \$pages_which,
	-value => 1
	)->pack(-side => 'left');
    $pages_frame1->Label(
	-text => 'All Pages'
	)->pack(-side => 'left');
    my $pages_frame2 = $pages_frame->Frame(
	)->pack(-side => 'top', -anchor => 'w');
    $pages_frame2->Radiobutton(
	-variable => \$pages_which,
	-value => 2
	)->pack(-side => 'left');
    $pages_frame2->Label(-text => 'Pages from'
	)->pack(-side => 'left');
    $pages_frame2->Entry(
	-width => 5,
	-background => 'white',
	-textvariable => \$self->{page_start},
	-validate => 'all',
	-validatecommand => sub {
		$pages_which = 2;
		return shift =~ /^([1-9]\d{0,4})?$/;
		}
	)->pack(-side => 'left');
    $pages_frame2->Label(-text => 'to'
	)->pack(-side => 'left');
    $pages_frame2->Entry(
	-width => 5,
	-background => 'white',
	-textvariable => \$self->{page_end},
	-validate => 'all',
	-validatecommand => sub {
		$pages_which = 2;
		return shift =~ /^([1-9]\d{0,4})?$/;
		}
	)->pack(-side => 'left');
    $pages_rb1->configure(-command => sub {
	$self->{page_start} = "";
	$self->{page_end} = "";
	});

    # Middle frame 1 right: Number of copies
    my $copies_frame = $mid_frame_1->LabFrame(-label => 'Copies', -labelside => 'top'
	)->pack(-side => 'left', -fill => 'y', -padx => 5, -pady => 5);
    $copies_frame->Label(-text => 'Number of Copies:'
	)->pack(-side => 'left', -anchor => 'n');
    $copies_frame->Entry(
	-width => 4,
	-background => 'white',
	-textvariable => \$self->{copies},
	-validate => 'all',
	-validatecommand => sub {
		$pages_which = 2;
		return shift =~ /^([1-9]\d{0,4})?$/;
		}
	)->pack(-side => 'left', -anchor => 'n');

    #======================================================================
    # Middle frame 2
    #======================================================================
    my $mid_frame_2 = $w->Frame(
	)->pack(-side => 'top', -fill => 'x');

    # Middle frame 2 left: Paper Source
    my $source_frame = $mid_frame_2->LabFrame(-label => 'Paper Source', -labelside => 'top'
	)->pack(-side => 'left', -fill => 'both', -padx => 5, -pady => 5, -expand => 1);
#    my $source_frame1 = $source_frame->Frame(
#	)->pack(-side => 'top', -anchor => 'w');
#    $source_frame1->Radiobutton(
#	)->pack(-side => 'left');
#    $source_frame1->Label(
#	-text => "By Medium:"
#	)->pack(-side => 'left');
#    $source_frame1->Optionmenu(
#	-justify => 'left',
#	)->pack(-side => 'left');
    my $source_frame2 = $source_frame->Frame(
	)->pack(-side => 'top', -anchor => 'w');
    $source_frame2->Radiobutton(
	)->pack(-side => 'left');
    $source_frame2->Label(
	-text => "By Tray:"
	)->pack(-side => 'left');
    $self->{inputslot_longname} = undef;
    $self->{inputslot_optionmenu} = $source_frame2->Optionmenu(
	-justify => 'left',
	-variable => \$self->{inputslot},
	-textvariable => \$self->{inputslot_longname}
	)->pack(-side => 'left');

    # Middle frame 2 right: Duplex
    my $duplex_frame = $mid_frame_2->LabFrame(-label => 'Duplex', -labelside => 'top'
	)->pack(-side => 'left', -fill => 'both', -padx => 5, -pady => 5, -expand => 1);
    $duplex_frame->Label(
	-text => "Duplex:"
	)->pack(-side => 'left');
    $self->{duplex_longname} = undef;
    $self->{duplex_optionmenu} = $duplex_frame->Optionmenu(
	-justify => 'left',
	-variable => \$self->{duplex},
	-textvariable => \$self->{duplex_longname}
	)->pack(-side => 'left', -anchor => 'n');

    #======================================================================
    # Initial value insertion
    #======================================================================

    # Use ppop to get a list of available queues and fill the select box.
    {
    my @qlist = ();
    my $control = new PPR::PPOP("all");
    foreach my $dest ($control->list_destinations_comments())
	{
	my($dest_name, $dest_type, $dest_comment) = @$dest[0, 1, 4];
	my $longname = "$dest_name - $dest_comment";
	$self->{dest_longname} = $longname if($dest_name eq $self->{dest});
	push(@qlist, [$longname, $dest_name]);
	}
    $control->destroy();
    $queue_optionmenu->configure(-options => \@qlist);
    }

    # Load the features of the current destination into the select
    # controls.
    $self->dest_features_load();

    #======================================================================
    # Wait until the window disappears, presumably because the user has
    # closed it using the window manager or has pressed [Cancel] or [Print].
    #======================================================================
    $w->waitWindow;
    undef $self->{window};

    #======================================================================
    # Build the print command
    #======================================================================
    my @args = ();

    print STDERR "User pressed $self->{pressed}\n";
    if($self->{pressed} eq "Print")
	{
	push(@args, "-d", $self->{dest});
	push(@args, "-e", "responder");
	push(@args, "--page-list" => "") if();
	push(@args, "-n", $self->{copies});
	push(@args, "--feature", "Duplex=$self->{duplex}") if($self->{duplex_longname} ne "");
	push(@args, "--feature", "InputSlot=$self->{inputslot}") if($self->{inputslot_longname} ne "");

	}

    return @args;
    }

#======================================================================
# Internal function
# This is called whenever the selected queue changes.
#======================================================================
sub dest_features_load
    {
    my $self = shift;

    my @duplex_list = (
		["None", "None"],
		["Long Edge", "DuplexNoTumble"],
		["Short Edge", "DuplexTumble"]
		);

    my @inputslot_list = (
		["Upper Tray", "Upper"],
		["Lower Tray", "Lower"]
		);

if($self->{dest} eq "dummy")
	{
	@duplex_list = ();
	@inputslot_list = ();
	}

    $self->{duplex_longname} = "";
    for my $i (@duplex_list)
	{
	my($longname, $option) = @$i;
	$self->{duplex_longname} = $longname if($option eq $self->{duplex});
	}
    $self->{duplex_optionmenu}->configure(-options => \@duplex_list);


    $self->{inputslot_longname} = "";
    for my $i (@inputslot_list)
	{
	my($longname, $option) = @$i;
	$self->{inputslot_longname} = $longname if($option eq $self->{inputslot});
	}
    $self->{inputslot_optionmenu}->configure(-options => \@inputslot_list);
    }
	
1;
