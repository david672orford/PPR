#! @PERL_PATH@ -wT
#
# mouse:~ppr/src/www/responder_speach.cgi.perl
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
# This CGI script is used to help a client response receiver.  The response
# receiver can feed the received information back to this CGI script, and
# this script will generate an appropriate sound file for it to play.
#

use lib "@PERL_LIBDIR@";
require 'paths.ph';
require 'cgi_data.pl';
require 'respond.ph';
require 'speach.pl';
require 'speach_response.pl';
require 'cgi_time.pl';

# Make sure the PPR sound files are installed.
if(! speach_soundfiles_installed())
{
require 'cgi_error.pl';
error_doc("Sound Files Missing", <<"EndOfMessage");
<p>The server administrator has not installed the PPR sound file distribution.
Until this is done, you will not be able to hear audible messages.</p>
EndOfMessage
exit 0;
}

# Decode the CGI variables which will tell use what sound files
# to use.
&cgi_read_data();

# Possibly select a voice from those for which sound files
# are available.
if(defined($data{voice}))
	{
	speach_set_voice($data{voice});
	}

# Copy selected CGI variables into the parameters list for the
# speach_ppr_response() function.
@values = (
		["JOBID", "jobid" => "x y 1 0 x"],
		["TIME", "time" => ""],
		["CODE", "code" => "0"],
		["PAGES", "pages" => "?"],
		["EXTRA", "extra" => "z"]
		);
my %args = ();
while(my $i = shift @values)
	{
	my ($name, $cgi_name, $default_value) = @$i;
	$args{$name} = cgi_data_move($cgi_name, $default_value);
	}

# Assemble a list of sound files.
my @playlist = speach_ppr_response(\%args, $data{silly_sounds});

# Try to keep it in the caches.
print "Expires: ", cgi_time_format(time() + 86400), "\n";

# Tell the browser to expect a Sun audio file.
print <<"EndOfHeader";
Content-Type: audio/basic

EndOfHeader

# Write a Sun audio file to stdout (which goes to the browser).
speach_cat_au(STDOUT, @playlist);

exit 0;
