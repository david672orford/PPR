#! /usr/bin/perl -wT
#
# mouse:~ppr/src/www/about.cgi.perl
# Copyright 1995--2003, Trinity College Computing Center.
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
# Last modified 17 December 2003.
#

use 5.005;
use lib "?";
use strict;
use vars qw{%data};
require 'cgi_data.pl';
require 'cgi_intl.pl';
require 'cgi_back.pl';
require 'cgi_time.pl';
use PPR;

# Initialize the internationalization libraries and determine the
# content language and charset.
my ($charset, $content_language) = cgi_intl_init();

# Parse the CGI variables.
&cgi_read_data();

# Did the user press a "Back" or "Close" button?
if(cgi_data_move("action", "") eq "Close")
	{
	cgi_back_doit();
	exit 0;
	}

my $title = H_("About PPR");

print "Expires: ", cgi_time_format(time() + 3600), "\n";

# Start the HTML document.
print <<"End10";
Content-Type: text/html;charset=$charset
Content-Language: $content_language
Vary: user-agent, accept-language

<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">
<html>
<head>
<title>$title</title>
<meta http-equiv="Content-Script-Type" content="text/javascript">
<link rel="icon" href="../images/icon-16.png" type="image/png">
<link rel="SHORTCUT ICON" href="../images/icon-16.ico">
<style type="text/css">
BODY {
	background-color: white;
	color: black;
	}
TD {
	font-size: 10pt;
	font-family: "Times New Roman" Times serif;
	}	
</style>
</head>
<body>
<form method="POST" action="$ENV{SCRIPT_NAME}">
<table width="100%">
<tr><td align="center"><img src="../images/pprlogo2-medium.png"></td></tr>
<tr><td>${\html($PPR::VERSION)}<br>
	Copyright 1995--2003, Trinity College Computing Center.<br>
	Written by David Chappell.<br>
	<a href="http://ppr.trincoll.edu" target="_blank">http://ppr.trincoll.edu</a><br>
	</td></tr>
<tr><td align="right">
End10

isubmit("action", "Close", _("Close"), _("Close this window."), "window.close()");

print <<"End100";
</td></tr>
</form>
</body>
</html>
End100

exit 0;

