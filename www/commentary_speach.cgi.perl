#! /usr/bin/perl -wT
#
# mouse:~ppr/src/www/commentary_speach.cgi.perl
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
# Last modified 10 June 2000.
#

use lib "?";
require 'paths.ph';
require 'cgi_data.pl';
require 'speach.pl';
require 'speach_commentary.pl';
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

# Choose defaults (some of then quite absurd) for missing CGI query values.
$data{printer} = "" if(!defined($data{printer}));
$data{category} = 0 if(!defined($data{category}));
$data{cooked} = "" if(!defined($data{cooked}));
$data{raw1} = "" if(!defined($data{raw1}));
$data{raw2} = "" if(!defined($data{raw2}));
$data{severity} = 5 if(!defined($data{severity}));
$data{silly_sounds} = 0 if(!defined($data{silly_sounds}));

# Assemble a list of sound files.
my @playlist = speach_ppr_commentary($data{printer}, $data{category}, $data{cooked}, $data{raw1}, $data{raw2}, $data{severity}, $data{silly_sounds});

# Try to keep it in the caches.
print "Expires: ", cgi_time_format(time() + 86400), "\n";

# Tell the browser to expect a Sun audio file.
print <<"EndOfHeader";
Content-Type: audio/basic

EndOfHeader

# Write a Sun audio file to stdout (which goes to the browser).
speach_cat_au(STDOUT, @playlist);

exit 0;

