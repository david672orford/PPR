#! /usr/bin/perl -wT
#
# mouse:~ppr/src/www/show_queues_nojs.cgi.perl
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
# Last modified 28 February 2001.
#

use lib "?";
require 'cgi_data.pl';
require 'cgi_intl.pl';
require 'cgi_time.pl';

my ($charset, $content_language) = cgi_intl_init();

&cgi_read_data();

my $name = $data{name};
my $type = $data{type};

defined($name) || die;
defined($type) || die;

my $encoded_HIST = form_urlencoded("HIST", $data{HIST});
my $encoded_name = form_urlencoded("name", $name);
my $encoded_type = form_urlencoded("type", $type);

my $html_title;
if($type eq "printer")
    {
    $html_title = html(sprintf(_("Printer \"%s\""), $name));
    }
elsif($type eq "group")
    {
    $html_title = html(sprintf(_("Group \"%s\""), $name));
    }
else
    {
    die;
    }

my $html_text = <<"HTML1";
<html>
<head>
<title>$html_title</title>
</head>
<body>
<table border=1 hspace=100 vspace=100>
<tr><th>$html_title</th></tr>
<tr><td><a href="show_jobs.cgi?$encoded_name;$encoded_HIST">${\H_("View Queue")}</a></td></tr>
HTML1

if($type eq "printer")
{
$html_text .= <<"Printer";
<tr><td><a href="prn_control.cgi?$encoded_name;$encoded_HIST">${\H_("Printer Control")}</a></td></tr>
<tr><td><a href="prn_properties.cgi?$encoded_name;$encoded_HIST">${\H_("Printer Properties")}</a></td></tr>
<tr><td><a href="show_printlog.cgi?${\form_urlencoded("printer", $name)};$encoded_HIST">${\H_("Printlog")}</a></td></tr>
<tr><td><a href="delete_queue.cgi?$encoded_type&$encoded_name;$encoded_HIST">${\H_("Delete Printer")}</a></td></tr>
Printer
}

elsif($type eq "group")
{
$html_text .= <<"Group";
<tr><td><a href="grp_properties.cgi?$encoded_name;$encoded_HIST">${\H_("Group Properties")}</a></td></tr>
<tr><td><a href="show_printlog.cgi?${\form_urlencoded("queue", $name)};$encoded_HIST">${\H_("Printlog")}</a></td></tr>
<tr><td><a href="delete_queue.cgi?$encoded_type&$encoded_name;$encoded_HIST">${\H_("Delete Group")}</a></td></tr>
Group
}

else
{
die;
}

$html_text .= <<"Tail";
</table>
</body>
</html>
Tail

# Try to keep this thing in the cache for a day.
print "Expires: ", cgi_time_format(time() + 86400), "\n";
{
# Get the time of this CGI script itself.
my($mtime) = (stat($0))[9];
if(defined($mtime))
    {
    print "Last-Modified: ", cgi_time_format($mtime), "\n";
    }
}

# Emmit the HTTP header, including the length of the content
# accumulated in $html_text.
my $content_length = length($html_text);
print <<"endHTTPHeader";
Content-Type: text/html;charset=$charset
Content-Language: $content_language
Content-Length: $content_length
Vary: accept-language

endHTTPHeader

# Send the HTML.
print $html_text;

exit 0;

