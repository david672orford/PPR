#! @PERL_PATH@ -wT
#
# mouse:~ppr/src/www/test_cgi.cgi.perl
# Copyright 1995--2005, Trinity College Computing Center.
# Written by David Chappell.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 
# * Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
# 
# * Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE 
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
# POSSIBILITY OF SUCH DAMAGE.
#
# Last modified 13 January 2005.
#

use lib "@PERL_LIBDIR@";
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
