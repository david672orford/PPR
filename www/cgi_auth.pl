#
# mouse:~ppr/src/www/cgi_auth.pl
# Copyright 1995--2004, Trinity College Computing Center.
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
# Last modified 28 February 2004.
#

sub demand_authentication
{
my $content = <<"Auth10";
<html>
<head>
<title>${\H_("Authentication Required")}</title>
</head>
<body>
<h1>${\H_("Authentication Required")}</h1>

<p>${\H_("Your web browser failed to validate your username and password\n"
	. "using an MD5 Digest as defined by RFC 2117.  If you entered your\n"
	. "username and password correctly, it may be that your browser\n"
	. "doesn't support MD5 Digest authentication.")}</p>

<p>${\H_("If your server is running Linux and the web browser is running on\n"
	. "the server, try connecting to localhost rather than to the machine\n"
	. "name.  On Linux systems the PPR web server can determine the user\n"
	. "id of the web browser if the connection is to localhost.")}</p>

<p>${\H_("If your browser supports JavaScript, Cookies, and Frames, then\n"
	. "click on the \"Cookie Login\" button in PPR's Printer Control\n"
	. "Panel.  A window will pop up that will log you in using a\n"
	. "modified form of MD5 Digest authentication implemented with\n"
	. "JavaScript and a CGI script.")}</p>

</body>
</html>
Auth10

my $content_length = length($content);

print <<"Auth20";
Content-Type: text/html
Content-Length: $content_length
Status: 401

Auth20

print $content;
}

1;

