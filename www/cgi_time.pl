#
# mouse:~ppr/src/www/cgi_time.pl
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

#
# Return an RFC 1123 time string for the indicated time.
# This is the format in which HTTP servers are required to
# state absolute time.
#
sub cgi_time_format
	{
	my $time_in = shift;

	my($sec,$min,$hour,$mday,$mon,$year,$wday) = (gmtime($time_in))[0 .. 6];

	$year += 1900;
	$mon = (qw(Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec))[$mon];
	$wday = (qw(Sun Mon Tue Wed Thu Fri Sat))[$wday];

	return sprintf("%s, %02d %s %d %02d:%02d:%02d GMT", $wday, $mday, $mon, $year, $hour, $min, $sec);
	}

#
# Parse an HTTP time string and return it in Unix format.
#
%cgi_time_months =
		(
		"Jan" => 0,
		"Feb" => 1,
		"Mar" => 2,
		"Apr" => 3,
		"May" => 4,
		"Jun" => 5,
		"Jul" => 6,
		"Aug" => 7,
		"Sep" => 8,
		"Oct" => 9,
		"Nov" => 10,
		"Dec" => 11
		);
sub cgi_time_parse
	{
	my $time_str = shift;

	require "Time/Local.pm";

	my($mday, $mon, $year, $hour, $min, $sec) = (undef, undef, undef, undef, undef, undef);

	# RFC 1123
	if($time_str =~ /^\w{3}, (\d{2}) (\w{3}) (\d{4}) (\d{2}):(\d{2}):(\d{2}) GMT$/)
		{
		($mday, $mon, $year, $hour, $min, $sec) = ($1, $cgi_time_months{$2}, ($3 - 1900), $4, $5, $6);
		}

	if(defined($mon))
		{
		return Time::Local::timegm($sec, $min, $hour, $mday, $mon, $year);
		}
	else
		{
		return undef;
		}
	}

1;
