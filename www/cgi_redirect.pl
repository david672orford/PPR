#
# mouse:~ppr/src/www/cgi_redirect.pl
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
# Last modified 9 June 2000.
#

sub cgi_redirect
	{
	my $location = shift;
	print "Location: $location\n";
	print "Content-Length: 0\n";
	print "\n";
	}

1;

