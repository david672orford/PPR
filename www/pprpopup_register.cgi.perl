#! /usr/bin/perl -w
#
# mouse:~ppr/src/templates/main.perl
# Copyright 1995--2001, Trinity College Computing Center.
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
# Last modified 18 December 2001.
#

use lib "?";
use PPR;
require "cgi_data.pl";

my $dbdir = "$PPR::VAR_SPOOL_PPR/pprpopup.db";

&cgi_read_data();
my $client=cgi_data_move("client", "");
my $pprpopup_address=cgi_data_move("pprpopup_address", "");
my $magic_cookie=cgi_data_move("magic_cookie", "");

$client =~ s/([^a-zA-Z0-9\@\.])/sprintf("%%%02X", ord $1)/ge;

if($pprpopup_address =~ /^0\.0\.0\.0:(\d+)$/)
    {
    $pprpopup_address = "$ENV{REMOTE_ADDR}:$1";

    } 

open(OUT, ">$dbdir/$client") || die $!;
print OUT "pprpopup_address=$pprpopup_address\n";
print OUT "magic_cookie=$magic_cookie\n";
close OUT;

print <<"EndHead";
Content-Type: text/plain
Content-Length: 0

EndHead

exit 0;

