#! /usr/bin/perl -wT
#
# mouse:~ppr/src/www/docs_cat.cgi.perl
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
# Last modified 24 April 2001.
#

#
# This CGI program simply copies the file to the web browser.  If the query
# variable "mime" is defined, then that is used as the MIME type, otherwise
# the MIME type is guessed at.
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

my $mime;

# This first try block catches errors in operations which must be done before the
# HTTP header is generated.  If an error is caught, it produces an error document.
eval {
    my $path = docs_open($ENV{PATH_INFO});
    $mime = cgi_data_move("mime", undef);
    $mime = guess_mime($path) if(! defined $mime);
    docs_last_modified(DOC, $path);
    };
if($@)
    {
    my $message = $@;
    require 'cgi_error.pl';
    error_doc(_("Can't Display File"), html($message), $charset, $content_language);
    exit 0;
    }

# From this point there is no going back.  We don't understand this MIME
# type, so there is no way to display errors in the browser.
print "Content-Type: $mime\n";
print "\n";

while(<DOC>)
    {
    print;
    }

close(DOC) || die $!;

exit 0;
