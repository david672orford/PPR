#
# mouse:~ppr/src/libscript/responder_argv.pl
# Copyright 1995--1999, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appears in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is"
# without express or implied warranty.
#
# Last modified 19 November 1999.
#

#
# This function convert the responder options into a hash.	That way if I
# ever change the responder options again I need only change this function
# in order to fix all of the Perl responders.
#
sub responder_argv
	{
	my $i;
	my %hash;
	foreach $i (qw(USER ADDRESS MESSAGE MESSAGE2 OPTIONS CODE JOBID EXTRA TITLE TIME WHY_ARRESTED PAGES CHARGE))
		{
		$hash{$i} = shift @_;
		}
	return \%hash;
	}

1;

