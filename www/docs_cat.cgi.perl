#! @PERL_PATH@ -wT
#
# mouse:~ppr/src/www/docs_cat.cgi.perl
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
# This CGI program simply copies the file to the web browser.  If the query
# variable "mime" is defined, then that is used as the MIME type, otherwise
# the MIME type is guessed at.
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

my $mime;

# This first try block catches errors in operations which must be done before the
# HTTP header is generated.	 If an error is caught, it produces an error document.
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
