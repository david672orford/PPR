#! /usr/bin/perl

# Last modified 30 October 2001.

# This filter sorts a .po file by filename and line number.

$/ = "\n\n";

my @entries = <STDIN>;

print shift @entries;

@entries = sort compare_function @entries;

print join("", @entries);

exit 0;

sub compare_function
	{
	my($name1, $line1) = $a =~ /#: ([^:]+):(\d+)/;
	my($name2, $line2) = $b =~ /#: ([^:]+):(\d+)/;
	my $filename_compare = $name1 cmp $name2;
	if($filename_compare == 0)
		{
		return $line1 <=> $line2;
		}
	else
		{
		return $filename_compare;
		}
	}

