#! /usr/bin/perl -wT
#
# mouse:~ppr/src/www/show_log.cgi.perl
# Copyright 1995--2000, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is"
# without express or implied warranty.
#
# Last modified 15 September 2000.
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

