#! /usr/bin/perl -wT
#
# mouse:~ppr/src/www/job_public.cgi.perl
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
# Last modified 5 April 2003.
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

	eval {
		if($username eq "")
			{
			die "You didn't enter your username.\n";
			}

		# Split the password (as encoded by the HTML form) into the nonce
		# and response portions.
		$password =~ /^(\S+) (\S+)$/ || die;
		my($nonce, $response) = ($1, $2);

		# Get the password from the password file.
		my $H1 = digest_getpw($username);

		# Compute a correct response based on the correct password.
		my $correct_response = md5hex("$H1:$nonce");

		if($response ne $correct_response)
			{
			die "Password is incorrect."
			}

		if(!digest_nonce_validate($domain, $nonce))
			{
			die "MD5 digest authentication nonce was too stale.  Please try again.";
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
<body bgcolor="#a4b6dd" fgcolor="black">
<form method="POST" action="$ENV{SCRIPT_NAME}">
Head

eval {

	#
	# [Cancel Job] pressed screen.
	#
	if($action eq "Cancel Job")
		{
		require "cgi_run.pl";
		run_or_die($PPR::PPOP_PATH, "--magic-cookie", $magic_cookie, "cancel", $jobname);
		print "<p>The job has been canceled.</p>\n";
		print "<p><input type=\"button\" value=\"Dismiss\" onclick=\"window.close()\"></p>\n";
		print "<script>window.close()</script>\n";
		}

	else
		{
		my $error = undef;

		# If [OK] pressed with a valid username and password pair,
		if(($action eq "OK" || $username ne "") && !defined($error = validate_password($protection_domain, $username, $password)))
			{
			require "cgi_run.pl";
			run_or_die($PPR::PPOP_PATH, "--magic-cookie", $magic_cookie, "modify", $jobname, "for=$username", "question=");
			run_or_die($PPR::PPOP_PATH, "--magic-cookie", $magic_cookie, "release", $jobname);
			print "<p>Changed owner and released job.</p>\n";
			print "<p><input type=\"button\" value=\"Dismiss\" onclick=\"window.close()\"></p>\n";
			print "<script>window.close()</script>\n";
			}

		# First time or invalid login.	We will distinguish them in a momement.
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
