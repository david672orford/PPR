#! @PERL_PATH@ -wT
#
# mouse:~ppr/src/www/commentary_speach.cgi.perl
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

use lib "@PERL_LIBDIR@";
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

