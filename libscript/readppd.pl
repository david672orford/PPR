#
# mouse:~ppr/src/libscript/readppd.pl
# Copyright 1995--2004, Trinity College Computing Center.
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
# Last modified 19 December 2004.
#

require "paths.ph";

package readppd;

@nest_stack = ();
$current_handle = undef;

#
# This function uses the PPD file index (if it exists) to convert a PPD name
# to an actual file name with complete path.
#
sub _ppd_find_file
	{
	my $ppdname = shift;
	defined $ppdname || die;

	# if is already an absolute path
	if($ppdname =~ m#^/#)
		{
		return $ppdname;
		}

	# Search for it in the index creadeb by "ppr-index ppds".
	if(open(INDEX, $main::PPD_INDEX))
		{
		my $filename = undef;
		while(<INDEX>)
			{
			my($f_modelname, $f_filename) = (split(/:/))[0,1];
			if($f_modelname eq $ppdname)
				{
				$filename = $f_filename;
				last;
				}
			}
		close(INDEX) || die $!;
		return $filename if(defined $filename);
		}

	# Take a guess.
	return "$main::PPDDIR/$ppdname";
	}

#
# Open a PPD file.  The actual filename must be specified.  This is
# called by ppd_open() after it has resolved the PPD name to a filename
# and by ppd_readline() to handle include files.
#
sub _ppd_open
	{
	my $filename = shift;

	eval
		{
		(scalar @nest_stack < 10) || die "PPD files nested too deep.\n";

		if($filename !~ /^\//)					# if doesn't begin with slash,
			{
			(scalar @nest_stack > 0) || die;
			my $dirname = $nest_stack[0]->[1];
			$dirname =~ s#/[^/]+$##;
			$filename = "$dirname/$filename";
			}

		$current_handle = "PPDFILE" . scalar @nest_stack;

		if($filename =~ /^(.+\.gz)$/)
			{
			my $f = $1;
			$pid = open($current_handle, "-|");
			defined($pid) || die "Can't fork, $!";
			if($pid == 0)
				{
				exec("gunzip", "-c", $f);
				exit(255);
				}
			}
		else
			{
			open($current_handle, "<$filename") || die "Can't open \"$filename\", $!\n";
			}
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

#
# This is the public function for opening a PPD file.  The file may be selected
# by ModelName or by a path starting with slash.
#
sub main::ppd_open
	{
	my $ppdname = shift;
	(scalar @nest_stack == 0) || die;
	my $filename = _ppd_find_file($ppdname);
	_ppd_open($filename);
	return $filename;
	}

#
# How many levels deep are we into the include tree?  The value 1 (one)
# indicates that we are in the original file.
#
sub main::ppd_level
	{
	return scalar @nest_stack;
	}

#
# This function gets the next line from the PPD file.  Include files are handled
# transparently.
# 
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
#	 {
#	 print "\"$line\"\n";
#	 }

1;
