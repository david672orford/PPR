#! /usr/bin/perl -w
#
# mouse:~ppr/src/www/grp_control.cgi.perl
# Copyright 1995--2002, Trinity College Computing Center.
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
# Last modified 20 November 2002.
#

use lib "?";
require 'paths.ph';
require "cgi_data.pl";
require "cgi_intl.pl";
require "cgi_run.pl";

defined($PPOP_PATH) || die;

cgi_read_data();

my ($charset, $content_language) = cgi_intl_init();

my $name = cgi_data_move("name", "all");

print <<"Head10";
Content-Type: text/html;charset=$charset
Content-Language: $content_language
Vary: accept-language

<html>
<head>
Head10

# If we are the frame,
if(!defined $data{members})
    {
    opencmd(PPOP, $PPOP_PATH, "-M", "status", $name) || die;
    my @members = ();
    while(<PPOP>)
	{
	my($queue, $status) = split(/\t/, $_);
	push(@members, $queue);
	}
    close(PPOP) || die;

    my $title = html(sprintf(_("Status of \"%s\" Member Printers"), $name));
    print <<"Head20";
<title>$title</title>
</head>
<frameset rows="50,*" scrolling="no">
Head20

    print "<frame src=\"$ENV{SCRIPT_NAME}?", form_urlencoded("members", join(' ', @members)), "\">\n";

    my $encoded_HIST = form_urlencoded("HIST", $data{HIST});
    print "<frame name=\"bottom\" src=\"prn_control.cgi?$encoded_HIST;", form_urlencoded("name", $members[0]), "\">\n";

    print "</frameset>\n";
    }

# If we are the top frame,
else
    {
print <<"Head30";
</head>
<style>
BODY {
	background-color: yellow; color: black;
	margin: 5mm 0mm 0mm 5mm;
	}
SPAN.button {
	background: #a0a0a0;
	color: black;
	padding: 2mm;
	border: medium solid black;
	-moz-border-radius: 3mm 3mm 0mm 0mm;
	margin: 0mm 5mm 0mm 0mm;
	}
A {
	text-decoration: none;
	}
</style>
<body>
Head30

    foreach my $member (split(/\s+/, cgi_data_move("members", "")))
	{
	print "<a target=\"bottom\" href=\"prn_control.cgi?", form_urlencoded("name", $member), "\">\n";
	print "<span class=\"button\">\n";
	print "\t", html($member), "\n";
	print "</span>\n";
	print "</a>\n\n";
	}
    print "</body>\n";
    }

print "</html>\n";

exit 0;
