#
# mouse:~ppr/src/libscript/readppd.pl
# Copyright 1995--2002, Trinity College Computing Center.
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
# Last modified 8 August 2002.
#

require "paths.ph";

package readppd;

@nest_stack = ();
$current_handle;

sub _ppd_open
    {
    my $filename = shift;

    eval
	{
        (scalar @nest_stack < 10) || die "PPD files nested too deep.\n";

        if($filename !~ /^\//)			# if doesn't begin with slash,
            {
            if(scalar @nest_stack == 0)		# if not an include file
                {
                $filename = "$main::PPDDIR/$filename";
                }
            else
                {
                my $dirname = $nest_stack[0]->[1];
                $dirname =~ s#/[^/]+$##;
                $filename = "$dirname/$filename";
                }
            }

        $current_handle = "PPDFILE" . scalar @nest_stack;
        open($current_handle, "<$filename") || die "Can't open \"$filename\", $!\n";
	};

    if($@)
	{
	foreach my $i (@nest_stack)
	    {
	    close $i->[0];
	    }
	die $@;
	}

    unshift @nest_stack, [$current_handle, $filename];
    }

sub main::ppd_open
    {
    my $filename = shift;
    (scalar @nest_stack == 0) || die;
    _ppd_open($filename);
    }

sub main::ppd_readline
    {
    my $line;
    while(1)
	{
        while(scalar(@nest_stack) > 0 && !defined($line = <$current_handle>))
            {
            close($current_handle) || die $!;
            shift @nest_stack;
            return undef if(scalar @nest_stack == 0);
            $current_handle = $nest_stack[0]->[0];
            }
	chomp $line;
	if($line =~ /^\*Include:\s*"([^"]+)"/)
	    {
	    _ppd_open($1);
	    next;
	    }
	last;
	}
    return $line;
    }

#package main;
#use lib "/usr/lib/ppr/lib";
#ppd_open("QMS-PS 410 DSC");
#while(defined ($line = ppd_readline()))
#    {
#    print "\"$line\"\n";
#    }

1;
