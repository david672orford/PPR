#! /usr/bin/perl -wT
#
# mouse:~ppr/src/www/show_log.cgi.perl
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
# Last modified 25 November 2002.
#


use lib "?";
require "paths.ph";
require "cgi_data.pl";
require "cgi_intl.pl";
require "cgi_time.pl";
defined($LOGDIR) || die;

# How far into the future (in seconds) should we declare the expiration date
# to be?
$LIFETIME = 60;

# Initialize the internationalization libraries and determine the
# content language and charset.
my ($charset, $content_language) = cgi_intl_init();

# Read the CGI form variables.
&cgi_read_data();

# Which log file should we display?
my $filename = cgi_data_move('name', 'x');

# Clean out unlikely characters to prevent tricks like using "..".
$filename =~ /^([a-z0-9-]+)/i;
$filename = $1;

# Fully qualify the fullname.
$filename = "$LOGDIR/$filename";

# The internationalized title of the document.
my $title = html(sprintf(_("PPR log \"%s\""), $filename));

# Make the last modification time of the HTML file be the same
# as that of the log file it was created from.
{
my($mtime) = (stat($filename))[9];
if(defined($mtime))
	{
	print "Last-Modified: ", cgi_time_format($mtime), "\n";
	print "Expires: ", cgi_time_format(time() + $LIFETIME), "\n";
	}
}

print <<"LogHead";
Content-Type: text/html;charset=$charset
Content-Language: $content_language
Vary: accept-language

<html>
<head>
<title>$title</title>
<link rel="parent" href="../html/show_logs.html" title="PPR Spooler Logs">
<link rel="top" href=".." title="PPR Spooler Management">
</head>
<body>
<h1>$title</h1>
LogHead

if(open(LOG, "< $filename"))
	{
	print "<pre>\n";
	while(<LOG>)
		{
		print html($_);
		}
	close(LOG);
	print "</pre>\n";
	}
else
	{
	print "<p>", html(sprintf(_("Can't read log file: %s"), $!)), "\n";
	}

print <<"LogTail";
<hr>
<p>
</body>
</html>
LogTail

exit 0;

