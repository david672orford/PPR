#! /usr/bin/perl -wT
#
# mouse:~ppr/src/www/job_log.cgi.perl
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
# Last modified 15 June 2000.
#


use lib "?";
require "paths.ph";
require "cgi_data.pl";
require "cgi_intl.pl";
require "cgi_time.pl";
require "cgi_run.pl";
defined($PPOP_PATH) || die;

# How far into the future (in seconds) should we declare the expiration date
# to be?
$LIFETIME = 60;

# Initialize the internationalization libraries and determine the
# content language and charset.
my ($charset, $content_language) = cgi_intl_init();

# Read the CGI form variables.
&cgi_read_data();

# Which log file should we display?
my $jobname = cgi_data_move('jobname', 'x-1');

# The internationalized title of the document.
my $title = html(sprintf(_("Log for Job \"%s\""), $jobname));

my $ok = opencmd(LOG, $PPOP_PATH, "-M", "log", $jobname);
my $ok_error = $!;

# Try to read a modification time from the "ppop log" output.  There should
# be one if a log was found.
my $line1 = undef;
if($ok)
    {
    $line1 = <LOG>;
    if($line1 =~ /^mtime: (\d+)$/)
    	{
    	my $mtime = $1;
	print "Last-Modified: ", cgi_time_format($mtime), "\n";
	print "Expires: ", cgi_time_format(time() + $LIFETIME), "\n";
    	$line1 = undef;
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

if($ok)
    {
    print "<pre>\n";
    print html($line1) if(defined($line1));
    while(<LOG>)
	{
	print html($_);
	}
    close(LOG);
    print "</pre>\n";
    }
else
    {
    print "<p>", html(sprintf(_("Can't read log file: %s"), $ok_error)), "\n";
    }

print <<"LogTail";
</body>
</html>
LogTail

exit 0;

