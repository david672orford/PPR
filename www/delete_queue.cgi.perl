#! /usr/bin/perl -wT
#
# mouse:~ppr/src/www/delete_queue.cgi.perl
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
# Last modified 17 December 2003.
#

use lib "?";
require 'cgi_data.pl';
require 'cgi_intl.pl';

defined($PPAD_PATH) || die;

if($ENV{REMOTE_USER} eq "")
	{
	require "cgi_auth.pl";
	demand_authentication();
	exit 0;
	}

# Initialize the internationalization libraries and determine the
# content language and charset.
my ($charset, $content_language) = cgi_intl_init();

# Read the query string and POST data.
&cgi_read_data();

# Which button did the user press?
my $action = cgi_data_move('action', undef);

# Second screen "No" or third screen "Close" are caught here.
# Actually, we never get here if the JavaScript onclick handlers worked.
if(defined($action) && ($action eq 'No' || $action eq 'Close'))
	{
	require 'cgi_back.pl';
	cgi_back_doit();
	exit 0;
	}

my $type = $data{type};
my $name = $data{name};

my $title;
if($type eq "printer")
	{ $title = sprintf(_("Delete Printer \"%s\""), $name) }
elsif($type eq "group")
	{ $title = sprintf(_("Delete Group \"%s\""), $name) }
elsif($type eq "alias")
	{ $title = sprintf(_("Delete Alias \"%s\""), $name) }
else
	{ $title = sprintf("Delete the %s \"%s\"", $type, $name) }
$title = html($title);

print <<"DelHead";
Content-Type: text/html;charset=$charset
Content-Language: $content_language
Vary: accept-language

<html>
<head>
<title>$title</title>
</head>
<body>
<form action="$ENV{SCRIPT_NAME}" method="POST">
DelHead

#
# First screen
#
if(!defined($action))
{
print "<p>\n";

if($type eq "printer")
	{ print html(sprintf(_("Delete the printer \"%s\"?"), $name)) }
elsif($type eq "group")
	{ print html(sprintf(_("Delete the group \"%s\"?"), $name)) }
elsif($type eq "alias")
	{ print html(sprintf(_("Delete the alias \"%s\"?"), $name)) }
else
	{ print html(sprintf("Delete the %s \"%s\"?", $type, $name)) }

isubmit("action", "Yes", N_("Yes"));
isubmit("action", "No", N_("No"), undef, "window.close()");

print "</p>\n";
}

#
# Second screen "Yes"
#
elsif($action eq 'Yes')
{
require 'paths.ph';
require 'cgi_run.pl';

my @PPAD = ($PPAD_PATH, "--su", $ENV{REMOTE_USER});
my $ret;

print "<pre>\n";
if($type eq "printer")
	{ $ret = run(@PPAD, "delete", $name) }
elsif($type eq "group")
	{ $ret = run(@PPAD, "group", 'delete', $name) }
elsif($type eq "alias")
	{ $ret = run(@PPAD, "alias", "delete", $name) }
else
	{ die "missing case" }
print "</pre>\n";

if($ret == 0)
	{
	print "<p>";
	if($type eq "printer")
		{ print H_("The printer has been deleted.") }
	elsif($type eq "group")
		{ print H_("The group has been deleted.") }
	elsif($type eq "alias")
		{ print H_("The alias has been deleted.") }
	else
		{ html(sprintf("The %s has been deleted.", $type)) }
	print "</p>\n";

	print "<script>opener.gentle_reload()</script>\n";
	}

isubmit("action", "Close", N_("_Close"), undef, "window.close()");
}

#
# Unrecognized button
#
else
{
print <<"Del4";
<p>Invalid action "$action".</p>
Del4
}

&cgi_write_data();

print <<"DelTail";
</form>
</body>
</html>
DelTail

exit 0;

