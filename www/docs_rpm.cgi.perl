#! /usr/bin/perl -wT
#
# mouse:~ppr/src/www/docs_rpm.cgi.perl
# Copyright 1995--2002, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appears in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is"
# without express or implied warranty.
#
# Last modified 11 April 2002.
#

use lib "?";
use PPR;
require "cgi_data.pl";
require "cgi_intl.pl";
require "docs_util.pl";

# Without this, rpm claims that it can't expand "~/.rpmrc".
$ENV{HOME} = ".";

# Without this Perl won't run rpm since the values are tainted.
defined($PPR::SAFE_PATH) || die;
$ENV{PATH} = $PPR::SAFE_PATH;
delete $ENV{ENV};

&cgi_read_data();
my $pkg = cgi_data_move("pkg", "");
my $html_title = html(sprintf(_("Documentation for RPM Package %s"), $pkg));

print <<"EndHead";
Content-Type: text/html

<html>
<head>
<title>$html_title</title>
</head>
<body>
<h1>$html_title</h1>
EndHead

eval {

# Unsure what the dangers are here.
$pkg =~ /^([^`\/]+)$/ || die;
$pkg = $1;

# Query for basic information and long description.
open(RPM, "rpm -q $pkg --queryformat \"%{NAME}\\t%{VERSION}\\t%{RELEASE}\\t%{SUMMARY}\\t%{URL}\\t%{DISTRIBUTION}\\t%{VENDOR}\\t%{SOURCE}\\t%{DESCRIPTION}\\n\" |") || die $!;

# Get basic information about the package.
my $line = <RPM>;
my($name, $version, $release, $summary, $url, $distribution, $vendor, $source, $description) = split(/\t/, $line);

# Read the long description.
while(<RPM>)
	{
	last if(/^%/);				# other languages than English
	$description .= $_;
	}

# We are done with that instance of rpm.  Ignore the exit code.
close(RPM) || die $!;

# Present the basic information in an HTML table.
print "<table border=1>\n";
print "<tr><th>Name</th><td>", html("$name-$version-$release"), "</td></tr>\n";
print "<tr><th>Summary</th><td>", html($summary), "</td></tr>\n";
print "<tr><th>URL</th><td><a href=\"$url\">$url</a></td></tr>\n";
print "<tr><th>Source</th><td><a href=\"$source\">$source</a></td></tr>\n";
print "<tr><th>Distribution</th><td>", html($distribution), "</td></tr>\n";
print "<tr><th>Vendor</th><td>", html($vendor), "</td></tr>\n";
print "</table>\n";

# And now the description as a paragraph.
print "<p>", html($description), "\n";

# Now run rpm again to list the documentation files.
open(RPM, "rpm -qdl $pkg |") || die $!;
print "<ul>\n";
while(my $file = <RPM>)
	{
	chomp $file;
	my $mime = docs_guess_mime($file);
	next if($mime =~ m#^image/#);

	my $encoded_path = $file;
	$encoded_path =~ s/([^a-zA-Z0-9 \/\.-])/sprintf("%%%02X",unpack("C",$1))/ge;

	if($mime eq "text/plain")
		{
		$url = "docs_plain.cgi$encoded_path";
		}
	elsif($mime eq "application/x-troff-man")
		{
		$url = "docs_man.cgi$encoded_path";
		}
	elsif($mime eq "application/x-info")
		{
		# The info viewer isn't designed to view individual files.
		#$url = "docs_info.cgi$encoded_path";
		$url = ("docs_cat.cgi$encoded_path?" . form_urlencoded("mime", $mime));
		}
	else
		{
		$url = ("docs_cat.cgi$encoded_path?" . form_urlencoded("mime", $mime));
		}

	print "<li><a href=\"$url\">", html($file), "</a>\n";
	}
print "</ul>\n";
close(RPM) || die $!;

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

