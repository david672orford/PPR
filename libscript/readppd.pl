#
# mouse:~ppr/src/libscript/readppd.pl
# Copyright 1995--2000, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appears in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is"
# without express or implied warranty.
#
# Last modified 14 December 2000.
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
        open($current_handle, "<$filename") || die "Can't open \"$filename\", $error\n";
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
