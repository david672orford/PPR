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
# Last modified 27 August 2002.
#

use lib "?";
use PPR;
require "cgi_data.pl";

# This is the directory in which we will store the registrations.
my $dbdir = "$PPR::VAR_SPOOL_PPR/pprpopup.db";

# This is our output.
my $content = "";
my $status = undef;

eval {

	# Set umask so that others can't read the files we create.
	umask(0007);

	# Get the CGI data.
	&cgi_read_data();

	# This is what the string which will be passed to ppr's -r option when jobs
	# from this client are submitted.
	my $client = cgi_data_move("client", undef);
	defined($client) || die "no client=";

	# This is the name or IP address and port number at which PPR Popup can be
	# reached.
	my $pprpopup_address = cgi_data_move("pprpopup_address", undef);
	defined($pprpopup_address) || die "no pprpopup_address=";

	# This is the password that gets us in.
	my $magic_cookie = cgi_data_move("magic_cookie", undef);
	defined($magic_cookie) || die "no magic_cookie-";

	# We convert the client name to lower case and change characters that are not
	# alpha-numberic and aren't the comercial at sign or period to percent plus
	# two hexadecimal digits.
	$client =~ tr/A-Z/a-z/;
	$client =~ s/([^a-zA-Z0-9\@\.])/sprintf("%%%02X", ord $1)/ge;

	# If the client claims its IP address is 0.0.0.0, that means it doesn't know
	# so substitute the address identified by the web server as the source address
	# of the request.
	if($pprpopup_address =~ /^0\.0\.0\.0:(\d+)$/)
		{
		$pprpopup_address = "$ENV{REMOTE_ADDR}:$1";
		} 

	# Write the information into the registration file.
	my $filename = "$dbdir/$client";
	open(OUT, ">${filename}_tmp") || die "can't open \"${filename}_tmp\": $!";
	print OUT "pprpopup_address=$pprpopup_address\n";
	print OUT "magic_cookie=$magic_cookie\n";
	close OUT || die "close() failed: $!";
	rename("${filename}_tmp", $filename) || die "rename(\"${filename}_tmp\", \"$filename\") failed: $!";

	# If the client identifier is in the form username@hostname, register an 
	# alias with the client IP address on the right hand side.	This will allow
	# response from jobs that arrive via the web interface.	 (At least Unix 
	# jobs.)
	if($client =~ /^([^\@]+)\@(.+)$/)
		{
		my($user, $host) = ($1, $2);
		if($host ne $ENV{REMOTE_ADDR})
			{
			my $filename2 = "$dbdir/$user\@$ENV{REMOTE_ADDR}";
			unlink($filename2);
			link($filename, $filename2) || die "link(\"$filename\", \"$filename2\") failed: $!";
			}
		}
	};
if($@)
	{
	$content = "Error: $@";
	$status = 500;
	}

print "Status: $status\n" if(defined $status);
print <<"EndHead";
Content-Type: text/plain
Content-Length: ${\length($content)}

EndHead
print $content;

# That's it!
exit 0;
