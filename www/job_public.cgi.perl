#! /usr/bin/perl -wT
#
# mouse:~ppr/src/www/job_public.cgi.perl
# Copyright 1995--2001, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is"
# without express or implied warranty.
#
# Last modified 7 December 2001.
#


use lib "?";
use PPR;
require "cgi_data.pl";
require "cgi_intl.pl";

# Initialize the internationalization libraries and determine the
# content language and charset.
my ($charset, $content_language) = cgi_intl_init();

# Read the CGI form variables.
&cgi_read_data();

my $jobname = cgi_data_peek("jobname", "???");
my $action = cgi_data_move("action", "");
my $username = cgi_data_move("username", "");
my $password = cgi_data_move("password", "");

my $title = html($jobname);

print <<"Head";
Content-Type: text/html;charset=$charset
Content-Language: $content_language
Vary: accept-language

<html>
<head>
<title>$title</title>
</head>
<body>
<h1>$title</h1>
<form method="POST" action="$ENV{SCRIPT_NAME}">
Head

eval {

if($action eq "" || ($action eq "OK" && $username eq ""))
{
print <<"Login";
<p>
Username: <input type="text" name="username" value="" size=16>
<br>
Password: <input type="password" name="password" value="" size=16>
</p>
<p>
<input type="submit" name="action" value="OK">
<input type="submit" name="action" value="Cancel Job">
</p>
Login

if($action ne "" && $username eq "")
    {
    print "<br><p><font color=\"red\" size=\"+2\">You didn't enter your username!</font></p>\n";
    }
}

elsif($action eq "OK")
{
require "cgi_run.pl";

print "Changing owner for job:<br>\n";
print "<pre>\n";
run($PPR::PPOP_PATH, "modify", $jobname, "for=$username");
print "</pre>\n";

print "Releasing job:<br>\n";
print "<pre>\n";
run($PPR::PPOP_PATH, "release", $jobname);
print "</pre>\n";
}

elsif($action eq "Cancel Job")
{
require "cgi_run.pl";
print "Canceling job:<br>\n";
print "<pre>\n";
run($PPR::PPOP_PATH, "scancel", $jobname);
print "</pre>\n";
}

else
{
die "Unrecognized action: $action\n";
}

}; if($@)
    {
    print "<p>Error: ", html($@), "</p>\n";
    }

cgi_write_data();

print <<"Tail";
</form>
</body>
</html>
Tail

exit 0;

