#! /usr/bin/perl -wT
#
# mouse:~ppr/src/commentary_select.cgi.perl
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
# Last modified 30 October 2001.
#

use lib "?";
use PPR::PPOP;
require 'cgi_time.pl';
require 'cgi_data.pl';
require 'cgi_intl.pl';

# Select best language for user.
my ($charset, $content_language) = cgi_intl_init();

# Get a translated title.
my $title = H_("PPR Printer Commentary Selection");

# Function which returns " selected" if this browser is likely
# to support Microsoft's Active X.
sub activex
    {
    if(defined $ENV{HTTP_USER_AGENT}
		&& $ENV{HTTP_USER_AGENT} =~ /; MSIE (\d+\.\d+);/
		&& $1 > 4.0
	)
    	{
    	return " selected";
    	}
    else
	{
	return "";
	}
    }

# Give a future expiration date so that the browser will not reload
# and destroy the form if we go back.  I need a better solution to
# this problem.
print "Expires: ", cgi_time_format(time() + 24 * 60 * 60), "\n";

print <<"QUOTE10";
Content-Type: text/html;charset=$charset
Content-Language: $content_language
Vary: accept-language

<html>
<head>
<title>$title</title>
<style>
BODY { background: white; color: black }
TABLE { border-style: solid; border-color: black }
SPAN.label { font-weight: bold }
INPUT.buttons { background: #a0a0a0 }
</style>
</head>
<body>
<h1>$title</h1>
<form action="/push/audio1" method="post">
<p>
<input class="buttons" type="reset" value="Defaults">
<input class="buttons" type="submit" value="Start">
</p>

<table border=2 cellpadding=5 cellspacing=0>
<tr>

<td>
<span class="label">${\H_("Event Categories:")}</span><br>
<input type="checkbox" name="categories" value="1" checked> ${\H_("Printer Errors")}<br>
<input type="checkbox" name="categories" value="2"> ${\H_("Printer Status")}<br>
<input type="checkbox" name="categories" value="4" checked> ${\H_("Printer Stalled")}<br>
<input type="checkbox" name="categories" value="8"> ${\H_("Printing Attempt Results")}<br>
</td>

<td valign="top">

<label><span class="label">${\H_("Severity Threshold:")}</span>
<select name="severity">
<option selected>1
<option>2
<option>3
<option>4
<option>5
<option>6
<option>7
<option>8
<option>9
<option>10
</select>
</label>

<br><label><span class="label">${\H_("Voice:")}</span>
<select name="voice">
<option value="male1" selected>male1
<option value="male1_44000">male1_44000
</select>
</label>

<br><label><span class="label">${\H_("Silly Sounds:")}</span>
<input type="radio" name="silly_sounds" value="1"> ${\H_("Yes")}
<input type="radio" name="silly_sounds" value="0" checked> ${\H_("No")}
</label>

<br><label><span class="label">${\H_("Player:")}</span>
<select name="method">
<option value="0">Hidden Frame
<option value="1">&lt;EMBED&gt; tag
<option value="2"${\activex()}>Windows Media Player Object
</select>
</label>

</td>
</tr>
</table>

QUOTE10

# List the printers with a checkbox next to each.
eval {
print "<p>", H_("To limit the commentary to certain printers, check one or\n"
		. "more or the boxes below."), "\n";

print <<"QUOTE30";
<table border=2 cellpadding=5 cellspacing=0>
<tr><th colspan=3>${\H_("Printers to Monitor")}</th></tr>
QUOTE30

my $control = new PPR::PPOP("all");

my %printers;
my $row;
foreach $row ($control->list_destinations_comments())
    {
    my($name, $type, $accepting, $protected, $comment) = @$row;
    next if($type ne "printer");
    $printers{$name} = $comment;
    }

my $name;
foreach $name (sort(keys(%printers)))
    {
    print "<tr><td><input type=\"checkbox\" name=\"printer\" value=", html_value($name), "></td>";
    print "<td>", html($name), "</td><td>", html($printers{$name}), "&nbsp;</td></tr>\n";
    }

print <<"QUOTE40";
</table>
QUOTE40

#
# Catch errors which may have accured during the listing
# of the printers.
#
}; if($@)
    {
    my $message = html($@);
    print "<p>$message</p>\n";
    }

print <<"TAIL10";
</form>
</body>
</html>
TAIL10

exit 0;

