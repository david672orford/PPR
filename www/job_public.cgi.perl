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
# Last modified 19 December 2001.
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
my $magic_cookie = cgi_data_peek("magic_cookie", "");
my $title = cgi_data_peek("title", "");
my $action = cgi_data_move("action", "");
my $username = cgi_data_move("username", "");
my $password = cgi_data_move("password", "");

print <<"Head";
Content-Type: text/html;charset=$charset
Content-Language: $content_language
Vary: accept-language

<html>
<head>
<title>${\html($jobname)}</title>
</head>
<body>
<form method="POST" action="$ENV{SCRIPT_NAME}">
Head

eval {

if($action eq "" || ($action eq "OK" && $username eq ""))
{
print "<p>", html(sprintf(
_("You have submitted a print job entitled \"%s\".  You may either enter your
username and password to print it or you may cancel it."), $title)),
"</p>\n";

print <<"Login";
<p>
Username: <input type="text" name="username" value="" size=16>
<br>
Password: <input type="password" name="password" value="" size=16 auth_md5="username printing 1234324:14314141afab">
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

print "Changing owner and releasing job:<br>\n";
print "<pre>\n";
run($PPR::PPOP_PATH, "--magic-cookie", $magic_cookie, "modify", $jobname, "for=$username", "question=");
run($PPR::PPOP_PATH, "--magic-cookie", $magic_cookie, "release", $jobname);
print "</pre>\n";
print "<input type=\"button\" value=\"Close\" onclick=\"window.close()\">\n";
}

elsif($action eq "Cancel Job")
{
require "cgi_run.pl";
print "Canceling job:<br>\n";
print "<pre>\n";
#run($PPR::PPOP_PATH, "--magic-cookie", $magic_cookie, "scancel", $jobname);
run($PPR::PPOP_PATH, "--magic-cookie", $magic_cookie, "cancel", $jobname);
print "</pre>\n";
print "<input type=\"button\" value=\"Close\" onclick=\"window.close()\">\n";
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

#==========================================================================
# What follows is taken from ppr-httpd.perl, pretty much unmodified.

$PWFILE = "$PPR::CONFDIR/htpasswd";
$SECRET = "jlk5rvnsdf8923sdklf";
$REALM = "printing";
$MAX_NONCE_AGE = 600;

# Hash a string using MD5 and return the hash in hexadecimal.
sub md5hex
    {
    my $string = shift;
    require "MD5pp.pm";
    return unpack("H*", MD5pp::Digest($string));
    }

# This function generates the hashed part of the server nonce.
sub digest_nonce_hash
    {
    my $nonce_time = shift;
    my $domain = shift;
    return md5hex("$nonce_time:$domain:$SECRET");
    }

# Find the user's entry (for the correct realm) in the private
# password file.
sub digest_getpw
    {
    my $sought_username = shift;
    my $answer = undef;
    open(PW, "<$PWFILE") || die "Can't open \"$PWFILE\", $!\n";
    while(<PW>)
	{
	chomp;
	my($username, $realm, $hash) = split(/:/);
	if($username eq $sought_username && $realm eq $REALM)
	    {
	    $answer = $hash;
	    last;
	    }
	}
    close(PW) || die;
    return $answer if(defined($answer));
    die "User \"$sought_username\" not in \"$PWFILE\"\n";
    }

