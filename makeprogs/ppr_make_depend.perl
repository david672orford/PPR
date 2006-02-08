#! /usr/bin/perl
#
# mouse:~ppr/src/makeprogs/ppr_make_depend.perl
# Copyright 1995--2006, Trinity College Computing Center.
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
# Last modified 23 September 2005.
#

defined($INCDIR = $ARGV[0]) || die;

if(!defined($SEARCHDIR = $ARGV[1]))
	{ $SEARCHDIR = "." }

opendir(D, $SEARCHDIR) || die "Can't open \"$SEARCHDIR\", $!";

open(OUT, ">$SEARCHDIR/.depend") || die;

while($file = readdir(D))
	{
	# Skip files which aren't C source code.
	next if($file !~ /^([^\.]+)\.c$/);

	print "Examining $file\n";

	open(F, "<$SEARCHDIR/$file") || die "Can't open \"$SEARCHDIR/$file\", $!";

	print OUT "$1.o: $SEARCHDIR/$file";

	while(<F>)
		{
		if(/^#include "([^"]+)"/)		# only #include "file.h" style
			{
			my $incfile = $1;
			if(-f "$INCDIR/$incfile")
				{ print OUT " $INCDIR/$incfile" }
			else
				{ print OUT " $incfile" }
			}
		}

	print OUT "\n\n";

	close(F) || die $!;
	}

closedir(D) || die $!;

close(OUT) || die $!;

exit(0);

