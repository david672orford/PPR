#! /usr/bin/perl -wT
#
# mouse:~ppr/src/www/test_components.cgi.perl
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
# Last modified 4 May 2001.
#

use lib "?";
require 'paths.ph';
require 'cgi_data.pl';
require 'cgi_intl.pl';

# Test for existence of the needed version of Perl.
sub test_perl5
    {
    return ($] >= 5.005);
    }

# Test for the existence of the Perl MD5 module.
sub test_md5
    {
    return defined(eval { require MD5 });
    }

# Test for the presence of the PPR sound files.
sub test_soundfiles
    {
    require "speach.pl";
    return speach_soundfiles_installed();
    }

# Test for the presence of the Perl GNU Gettext module.
sub test_gettext
    {
    return defined(eval { require Locale::gettext });
    }

# Test for the presence of the Perl Chart::PNGgraph module.
sub test_chart
    {
    return defined(eval { require Chart::PNGgraph });
    }

#
# This is a table of tests.
#
my @tests = (
	[\&test_perl5, H_("Perl 5.005 or later"), 1],
#	[\&test_md5, H_("Perl MD5 module"), 1],
	[\&test_soundfiles, H_("PPR sound files"), 0],
	[\&test_gettext, H_("Perl GNU Gettext module"), 0],
	[\&test_chart, H_("Perl Chart::PNGgraph module"), 0]
);

#
# Create the top of the page.
#
my $title = H_("Are Required Packages Present?");
my ($charset, $content_language) = cgi_intl_init();
print <<"Quote10";
Content-Type: text/html;charset=$charset
Content-Language: $content_language
Vary: accept-language

<html>
<head>
<title>$title</title>
<style type="text/css">
BODY { background: lightgrey; color: black; }
TH { text-align: left; padding: 1mm 8mm; }
TD { text-align: left; padding: 1mm 8mm; }
</style>
</head>
<body>
<h1>$title</h1>
<table border=1>
Quote10

#
# Print the column headings.
#
{
print "<tr>";
my $i;
foreach $i (H_("Component"), H_("Present?"), H_("Critical?"))
    {
    print "<th>$i</th>";
    }
print "</tr>\n";
}

#
# Run the tests and print the results.
#
{
my $test;
foreach $test (@tests)
    {
    my($funct, $desc, $critical) = @{$test};
    $found = &$funct ? H_("Yes") : H_("No");
    $critical = $critical ? H_("Yes") : H_("No");
    print "<tr><td>$desc</td><td>$found</td><td>$critical</td></tr>\n";
    }
}

#
# Close the HTML document.
#
print <<"Quote100";
</table>
</body>
</html>
Quote100

exit 0;

