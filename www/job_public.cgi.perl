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
# Last modified 20 December 2001.
#


use lib "?";
use PPR;
require "cgi_data.pl";
require "cgi_intl.pl";
require "cgi_digest.pl";

sub validate_password
    {
    my $domain = shift;
    my $username = shift;
    my $password = shift;
    
    if($username eq "")
    	{
	return "You didn't enter your username.\n";
    	}

    $password =~ /^(\S+) (\S+)$/ || die;
    my($nonce, $response) = ($1, $2);
    my $H1 = digest_getpw($username);
    my $correct_response = md5hex("$H1:$nonce");

    if($response ne $correct_response)
	{
	return "Password is incorrect." 
	}
	
    eval {
	if(!digest_nonce_validate($domain, $nonce))
	    {
	    return "MD5 digest authentication nonce was too stale.  Please try again.";
	    }
	};
    if($@)
    	{
	return $@;
    	}

    return undef;
    }
    
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

# Determine the protection domain for MD5 digestion authentication.
my $protection_domain = "http://$ENV{SERVER_NAME}:$ENV{SERVER_PORT}$ENV{SCRIPT_NAME}";
$protection_domain =~ s/[^\/]+$//;

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

    #
    # [Cancel Job] pressed screen.
    #
    if($action eq "Cancel Job")
	{
	require "cgi_run.pl";
	print "<p>The job has been canceled.</p>\n";
	run_or_die($PPR::PPOP_PATH, "--magic-cookie", $magic_cookie, "cancel", $jobname);
	print "<p><input type=\"button\" value=\"Dismiss\" onclick=\"window.close()\"></p>\n";
	}

    else
	{
	my $error = undef;
	
	# If [OK] pressed with a valid username and password pair,
	if($action eq "OK" && !defined($error = validate_password($protection_domain, $username, $password)))
	    {
	    require "cgi_run.pl";
	    run_or_die($PPR::PPOP_PATH, "--magic-cookie", $magic_cookie, "modify", $jobname, "for=$username", "question=");
	    run_or_die($PPR::PPOP_PATH, "--magic-cookie", $magic_cookie, "release", $jobname);
	    print "<p>Changed owner and released job.</p>\n";
	    print "<p><input type=\"button\" value=\"Dismiss\" onclick=\"window.close()\"></p>\n";
	    }

	# Anything else pressed or bad username or password,
	else
	    {
	    print "<p>", html(sprintf(
		_("You have submitted a print job entitled \"%s\".  You may either enter your\n"
		. "username and password to print it or you may cancel it."), $title)),
		"</p>\n";

	    print <<"Login";
<p>
Username: <input type="text" name="username" value=${\html_value($username)} size=16>
<br>
Password: <input type="password" name="password" value="" size=16 auth_md5="username $REALM ${\digest_nonce_create($protection_domain)}">
</p>
<p>
<input type="submit" name="action" value="OK">
<input type="submit" name="action" value="Cancel Job">
</p>
Login

	    # If there is a password error, print it here.
	    if(defined $error)
		{
		print "<br><p><font color=\"red\" size=\"+2\">", html($error), "</font></p>\n";
		}
	    }
	}


    # Catch exceptions and print them on the browser.
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
