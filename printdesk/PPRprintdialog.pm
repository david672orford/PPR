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
# Last modified 18 March 2003.
#

package PrintDesk::PPRprintdialog;
require 5.003;

sub new
    {
    shift;
    my $self = {};
    bless $self;
    $self->{window} = shift;
    return $self;
    }

sub Show
    {
    my $self = shift;

    my $w = $self->{window};

    my $right_buttons_frame = $w->Frame();
    $right_buttons_frame->pack(-side, 'bottom', -anchor, 'se');

    my $print_button = $right_buttons_frame->Button(-text, "Print");
    $print_button->pack(-side, 'right');

    my $cancel_button = $right_buttons_frame->Button(-text, "Cancel");
    $cancel_button->pack(-side, 'right');

    $w->waitWindow;
    undef $self->{window};

    return ();
    }

sub destroy
    {
    my $self = shift;
    $self->{window}->destroy() if(defined $self->{window});
    }

1;
