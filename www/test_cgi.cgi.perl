#! /usr/bin/perl -wT
#
# mouse:~ppr/src/www/test_cgi.cgi.perl
# Copyright 1995--2001, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is" without
# express or implied warranty.
#
# Last modified 16 January 2001.
#

use lib "?";
require "cgi_data.pl";

cgi_read_data();

print <<"QUOTE10";
Content-Type: text/html

<html>
<head>
<title>PPR CGI Test Page</title>
</head>
<body>

<h1>PPR CGI Test Results</h1>

<h2>Environment Variables</h2>
<p>
QUOTE10

my $name;
my $value;
foreach $name (sort(keys(%ENV)))
    {
    $value = $ENV{$name};
    $value =~ s/</&lt;/g;
    print "\$ENV{$name} = \"$value\"<br>\n";
    }

print <<"QUOTE20";
</p>

<h2>QUERY/POST Data</h2>
<p>
QUOTE20

foreach $name (sort(keys(%data)))
    {
    $value = $data{$name};
    $value =~ s/</&lt;/g;
    print "\$data{$name} = \"$value\"<br>\n";
    }

print <<"TAIL10";
</p>
</body>
</html>
TAIL10

exit 0;
