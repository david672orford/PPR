#! /usr/bin/perl -w
#
# mouse:~ppr/src/www/pprpopup_register.cgi.perl
# Copyright 1995--2002, Trinity College Computing Center.
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
# Last modified 22 February 2002.
#

use lib "?";
use PPR;
require "cgi_data.pl";

# This is the directory in which we will store the registrations.
my $dbdir = "$PPR::VAR_SPOOL_PPR/pprpopup.db";

# Set umask so that others can't read the files we create.
umask(0007);

# Get the CGI data.
&cgi_read_data();
my $client=cgi_data_move("client", "");
my $pprpopup_address=cgi_data_move("pprpopup_address", "");
my $magic_cookie=cgi_data_move("magic_cookie", "");

# We convert the client name to lower case and change characters
# that are not alpha-numberic and arn't at sign or period to
# percent plus two hexadecimal digits.
$client =~ tr/A-Z/a-z/;
$client =~ s/([^a-zA-Z0-9\@\.])/sprintf("%%%02X", ord $1)/ge;

# If the client claims its IP address is 0.0.0.0, it doesn't know.
# Substitute the address identified by the web server as the
# source address of the request.
if($pprpopup_address =~ /^0\.0\.0\.0:(\d+)$/)
    {
    $pprpopup_address = "$ENV{REMOTE_ADDR}:$1";
    } 

# Write the information into the registration file.
open(OUT, ">$dbdir/$client") || die $!;
print OUT "pprpopup_address=$pprpopup_address\n";
print OUT "magic_cookie=$magic_cookie\n";
close OUT;

# Return an empty document.
print <<"EndHead";
Content-Type: text/plain
Content-Length: 0

EndHead

# That's it!
exit 0;

