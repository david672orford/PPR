#! /usr/bin/perl

# Last modified 30 December 1999.

# This filter is used because GNU xgettext doesn't have a parser for Perl.
# The result isn't perfect, and it does produce some spurious warnings
# from xgettext, but it is good enough for now.

while(defined(my $file = shift(@ARGV)))
	{
	open F, "<$file" || die;
	print "# line 0 \"$file\"\n";
	while(<F>)
		{
		s/#.*$//;		# very crude and incorrect
		s/^\s*\.\s*"/"/;
		print;
		}
	}

