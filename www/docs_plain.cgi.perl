#! @PERL_PATH@ -wT
#
# mouse:~ppr/src/www/docs_plain.cgi.perl
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

#
# This CGI program reads a plain text file and converts it to HTML.	 While
# doing so, anything that looks like a URL or an email address is converted
# into a link.
#

use lib "@PERL_LIBDIR@";
use PPR;
require "cgi_data.pl";
require "cgi_intl.pl";
require "docs_util.pl";

# Prevent taint problems when running gzip and such.  See Programming 
# Perl 3rd edition, p. 565.
defined($ENV{PATH} = $PPR::SAFE_PATH) || die;
delete @ENV{qw(IFS CDPATH ENV BASH_ENV)};

# Read POST and query variables.
&cgi_read_data();

# Initialize the internationalization libraries and determine the
# content language and charset.
my ($charset, $content_language) = cgi_intl_init();

# This first try block catches errors in operations which must be done before the
# HTTP header is generated.	 If an error is caught, it produces an error document.
eval {
	my $path = docs_open(DOC, $ENV{PATH_INFO});
	$path =~ m#/([^/]+)$# || die;
	$html_title = html($1);
	docs_last_modified(DOC);
	};
if($@)
	{
	my $message = $@;
	require 'cgi_error.pl';
	error_doc(_("Can't Display File"), html($message), $charset, $content_language);
	exit 0;
	}

print <<"EndOfHead";
Content-Type: text/html

<html>
<head>
<title>$html_title</title>
</head>
<body>
<pre>
EndOfHead

# This is a second try block to catch errors that occur while
# generating the body.	It aborts the processing of the man
# page text and prints an error message as a paragraph.
eval {
	while(<DOC>)
		{
		my $line = html($_);

		# Turn HTTP and FTP URLs into hyperlinks.
		$line =~ s#\b(((http)|(ftp))://[^ \t\)&]+)#<a href=\"$1\">$1</a>#gi;

		# Turn email addresses into hyperlinks.
		$line =~ s#\b([^ \t\);]+\@[a-z0-9-]+(\.[a-z0-9-]+)+)\b#<a href=\"mailto:$1\">$1</a>#ig;

		# Here we try to take machine names and that begin with "www." and
		# "ftp." and add hyperlinks to URLs.  We must be careful not to
		# match real URLs.
		$line =~ s#(^|[^/])(www(\.[a-z0-9-]+)+)\b#<a href=\"http://$1\">$1$2</a>#ig;
		$line =~ s#(^|[^/])(ftp(\.[a-z0-9-]+)+)\b#<a href=\"ftp://$1\">$1$2</a>#ig;
		print $line;
		}

	close(DOC) || die $!;
	};
my $error = $@;
print "</pre>\n";
if($error)
	{
	print "<p>", H_("Error:"), " ", html($error), "</p>\n";
	}

print <<"EndOfTail";
</body>
</html>
EndOfTail

exit 0;
