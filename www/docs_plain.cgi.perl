#! /usr/bin/perl -wT
#
# mouse:~ppr/src/www/docs_plain.cgi.perl
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
# Last modified 30 October 2001.
#

#
# This CGI program reads a plain text file and converts it to HTML.  While
# doing so, anything that looks like a URL or an email address is converted
# into a link.
#

use lib "?";
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
# HTTP header is generated.  If an error is caught, it produces an error document.
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
# generating the body.  It aborts the processing of the man
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
