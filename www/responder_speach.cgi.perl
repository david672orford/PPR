#! /usr/bin/perl -wT
#
# mouse:~ppr/src/www/responder_speach.cgi.perl
# Copyright 1995--2000, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appears in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is"
# without express or implied warranty.
#
# Last modified 15 June 2000.
#

use lib "?";
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

