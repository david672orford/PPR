#! /usr/bin/perl -wT
#
# mouse:~ppr/src/www/docs_rpms_list.cgi.perl
# Copyright 1995--2001, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appears in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is"
# without express or implied warranty.
#
# Last modified 23 April 2001.
#

use lib "?";
use PPR;
require "cgi_data.pl";
require "cgi_intl.pl";

# Without this, rpm claims that it can't expand "~/.rpmrc".
$ENV{HOME} = ".";

# Without this Perl won't run rpm since the values are tainted.
$ENV{PATH} = $PPR::SAFE_PATH;
delete $ENV{ENV};

print <<"EndHead";
Content-Type: text/html

<html>
<head>
<title>RPM Packages Documentation Browser</title>
</head>
<body>
<h1>RPM Packages Documentation Browser</h2>
EndHead

eval {

my %groups = ();

open(RPM, "rpm -qa --queryformat \"%{GROUP}\\t%{NAME}-%{VERSION}\\t%{SUMMARY}\\n\" |") || die "open() failed, $!";
while(<RPM>)
	{
	my($group, $package, $summary) = split(/\t/);
	if(!exists $groups{$group})
		{
		$groups{$group} = [];
		}
	push(@{$groups{$group}}, [$package, $summary]);
	}
close(RPM) || die "close() failed, \$! = $!, \$? = $?";

foreach my $group (sort(keys %groups))
	{
	my $gp = $groups{$group};
	print "<h2>", html($group), "</h2>\n";
	print "<table border=1>\n";
	foreach $pp (@{$gp})
		{
		my($package, $description) = @{$pp};
		print "<tr>\n";
		print " <td><a href=\"docs_rpm.cgi?", form_urlencoded("pkg", $package), "\">", html($package), "</td>\n";
		print " <td>", html($description), "</td>\n";
		print "</tr>\n";
		}
	print "</table>\n";
	}

}; if($@)
	{
	my $error = $@;
	print "<p>", html($error), "\n";
	}

print <<"EndTail";
</body>
</html>
EndTail

exit 0;

