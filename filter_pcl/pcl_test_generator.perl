#! /usr/bin/perl -w
#
# PCL Test Generator
#
# Last modified 22 April 2002.
#

sub do_reset
	{
	print "\x1bE";
	}

sub do_pagesize
	{
	my $pagesize = shift;
	if($pagesize eq "Letter")
		{ print "\x1b&l2A" }
	elsif($pagesize eq "Legal")
		{ print "\x1b&l3A" }
	else
		{ die }
	}

sub do_bin
	{
	my $bin = shift;
	if($bin eq "main")
		{ print "\x1b&l1H" }
	elsif($bin eq "lower")
		{ print "\x1b&l4H" }
	elsif($bin eq "large")
		{ print "\x1b&l5H" }
	else
		{ die }
	}

sub do_formfeed
	{
	print "\x1b&l0H";
	}

do_reset();
#do_pagesize("Letter");
do_pagesize("Legal");
#do_bin("main");
#do_bin("lower");
print "Now is the time for all good men to come to the aid of the party.\r\n";
do_formfeed();
do_reset();

exit 0;
