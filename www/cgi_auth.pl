#
# mouse:~ppr/src/www/cgi_auth.pl
# Copyright 1995--2001, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appears in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is"
# without express or implied warranty.
#
# Last modified 2 January 2001.
#

sub demand_authentication
{
my $content = <<"Auth10";
<html>
<head>
<title>${\H_("Authorization Required")}</title>
</head>
<body>
<h1>${\H_("Authorization Required")}</h1>

<p>${\H_("Your web browser failed to validate your username and password using an MD5 Digest as defined 
by RFC 2117.  If you entered your username and password correctly, it may be that your browser
doesn't support MD5 Digest authentication.")}</p>

<p>${\H_("If your server is running Linux and the web browser is running on the server, try connecting
to localhost rather than to the machine name.  On Linux systems the PPR web server can determine
the user id of the web browser if the connection is to localhost.")}</p>

<p>${\H_("If your browser supports JavaScript, Cookies, and Frames, then click on the \"Cookie Login\" button
in PPR's Printer Control Panel.  A window will pop up that will log you in using a modified form
of MD5 Digest authentication implemented with JavaScript and a CGI script.")}</p>

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

