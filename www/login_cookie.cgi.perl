#! @PERL_PATH@ -wT
#
# mouse:~ppr/src/www/login_cookie.cgi.perl
# Copyright 1995--2005, Trinity College Computing Center.
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
# Last modified 13 January 2005.
#

use lib "@PERL_LIBDIR@";
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
print '<input type="button" value="', H_("Login"), '" onclick="login();return false">', "\n";
print '<input type="button" value="', H_("Close"), '" onclick="window.parent.close(self);return false">', "\n";
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
print '<input type="button" value="', H_("Close"), '" onclick="window.parent.close(self);return false">', "\n";
print "</p>\n";
}

print <<"EndofTail1";
</form>
</body>
</html>
EndofTail1

exit 0;
