#! /usr/bin/perl -wT
#
# mouse:~ppr/src/www/login_cookie.cgi.perl
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
# Last modified 20 December 2001.
#

use lib "?";
require "cgi_data.pl";
require "cgi_intl.pl";
require "cgi_digest.pl";

# These must be the same as in ppr-httpd.perl.
$SECRET = "jlkfjasdf8923sdklf";
$REALM="printing";
$MAX_NONCE_AGE = 600;

($charset, $content_language) = cgi_intl_init();

&cgi_read_data();

print <<"EndofHeader1";
Content-Type: text/html;charset=$charset
Content-Language: $content_language
Vary: accept-language

<html>
<head>
<meta http-equiv="Content-Script-Type" content="text/javascript">
<script type="text/javascript" src="../js/md5.js"></script>
<style type="text/css">
BODY { background: #d0d0d0; color: black; }
P.error { color: blue; }
</style>
</head>
<body>
<h1>${\H_("PPR Login")}</h1>
<form action="$ENV{SCRIPT_NAME}" method="POST">
EndofHeader1

# Genenate an MD5 Digest authentication server-side nonce.
{
my $domain = "http://$ENV{SERVER_NAME}:$ENV{SERVER_PORT}$ENV{SCRIPT_NAME}";
$domain =~ s/[^\/]+$//;
my $nonce = digest_nonce_create($domain);
#print '<input type="hidden" name="domain" value=', html_value($domain), ">\n";
print '<input type="hidden" name="nonce" value=', html_value($nonce), ">\n";
}

# Put the realm (probably "printing") in a hidden field since it is
# among the things which must be hashed in the response.
print '<input type="hidden" name="realm" value=', html_value($REALM), ">\n";

#=============================================================================
# If not logged in,
#=============================================================================
if(! defined $ENV{REMOTE_USER} || $ENV{REMOTE_USER} eq "")
{
print <<"EndLoginScript";
<script>
function login()
	{
	var myform = document.forms[0];
	var saveform = window.parent.frames[1].document.forms[0];
	saveform.ha1.value = MD5(myform.username.value + ':' + myform.realm.value + ":" + myform.password.value);
	myform.password.value = "";
	saveform.response.value = MD5(saveform.ha1.value + ':' + myform.nonce.value);
	document.cookie = "auth_md5=" + myform.username.value + ' ' + myform.nonce.value + ' ' + saveform.response.value;
	myform.submit();
	}
</script>
EndLoginScript
print "<p>\n";
print H_("Username:"), ' <input name="username" value=', html_value(cgi_data_move("username", "")), " size=16>\n";
print "<br>\n";
print H_("Password:"), ' <input name="password" value="" size=16 type="password">', "\n";
print "<br>\n";
print '<input type="button" value="', H_("Login"), '" onclick="login()">', "\n";
print '<input type="button" value="', H_("Close"), '" onclick="window.parent.close(self)">', "\n";
print "</p>\n";

# If the user tried to log in and failed,
if(cgi_data_peek("nonce", "") ne "")
	{
	print '<p class="error">', H_("Incorrect username or password."), "</p>\n";
	}
}

#=============================================================================
# If already logged in by cookie,
#=============================================================================
elsif($ENV{AUTH_TYPE} eq "Cookie")
{
print <<"LogoutScript";
<script>
function logout()
	{
	var saveform = window.parent.frames[1].document.forms[0];
	saveform.ha1.value = "";
	saveform.response.value = "";
	document.cookie = "auth_md5=";
	document.forms[0].nonce.value = "";
	document.forms[0].submit();
	}
</script>
LogoutScript
print '<input type="hidden" name="username" value=', html_value(cgi_data_move("username", "")), " size=16>\n";
print "<p>", html(sprintf(_("You are logged in as \"%s\".  Leave this window open to remain logged in.\n"
		. "If you close this window without logging out, you will be automatically logged\n"
		. "out after %d seconds.\n"), $ENV{REMOTE_USER}, $MAX_NONCE_AGE)), "</p>\n";
print '<p><input type="submit", name="renew" value="', H_("Renew"), '">', "\n";
print '<input type="button", value="', H_("Logout"), '" onclick="logout()">', "</p>\n";
print <<"LogoutScript2";
<script>
var myform = document.forms[0];
var saveform = window.parent.frames[1].document.forms[0];
saveform.response.value = MD5(saveform.ha1.value + ':' + myform.nonce.value);
document.cookie = "auth_md5=" + myform.username.value + ' ' + myform.nonce.value + ' ' + saveform.response.value;
window.setTimeout("document.forms[0].renew.click()", $MAX_NONCE_AGE * 1000 * 0.5);
</script>
LogoutScript2
}

#=============================================================================
# If already logged in by some other authentication method,
#=============================================================================
else
{
print "<p>\n";
print html(sprintf(_("You are already logged in as user \"%s\" using authentication method \"%s\"."), $ENV{REMOTE_USER}, $ENV{AUTH_TYPE}));
print "</p>\n";
print "<p>\n";
print '<input type="button" value="', H_("Close"), '" onclick="window.parent.close(self)">', "\n";
print "</p>\n";
}

print <<"EndofTail1";
</form>
</body>
</html>
EndofTail1

exit 0;
