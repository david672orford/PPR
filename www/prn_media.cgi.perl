#! /usr/bin/perl -wT
#
# mouse:~ppr/src/www/prn_media.cgi.perl
# Copyright 1995--2002, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is"
# without express or implied warranty.
#
# Last modified 22 February 2002.
#

use lib "?";
use PPR;
use PPR::PPOP;
require 'cgi_data.pl';
require 'cgi_intl.pl';

# Initialize the internationalization libraries and determine the
# content language and charset.
my ($charset, $content_language) = cgi_intl_init();

# Read the query string and POST data.
&cgi_read_data();

# Which printer are we working on?
my $name = cgi_data_move("name", "?");

# Which button did the user press?
my $action = cgi_data_move('action', undef);

# Build and internationalized title.
my $title = html(sprintf(_("Media Mounted on Printer \"%s\""), $name));

# If the user press the "Close" button and it got past
# JavaScript, try to load the previous page.
if(defined($action) && $action eq "Close")
    {
    require 'cgi_back.pl';
    cgi_back_doit();
    exit 0;
    }

# Will this operation require privileges?
if(defined($action) && $action eq "Close")
    {
    require 'cgi_back.pl';
    cgi_back_doit();
    exit 0;
    }

if($ENV{REMOTE_USER} eq "" && $action ne "")
    {
    require "cgi_auth.pl";
    demand_authentication();
    exit 0;
    }

print <<"Head";
Content-Type: text/html;charset=$charset
Content-Language: $content_language
Vary: accept-language

<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd">
<html>
<head>
<title>$title</title>
<link rel="stylesheet" href="../style/prn_media.css" type="text/css">
</head>
<body>
<h1>$title</h1>
<form action="$ENV{SCRIPT_NAME}?${\form_urlencoded("name", $name)}" method="POST">
Head

eval
{
# Get a list of the available media types.
open(M, $PPR::MEDIAFILE) || die $!;
print "<pre>\n";
my @available = ();
while(read(M, my $record, $PPR::SIZEOF_struct_Media) == $PPR::SIZEOF_struct_Media)
    {
    my($medianame, $width, $height, $weight, $colour, $type, $flag_suitability) =
    	unpack("A16dddA16A16i", $record);
    push(@available,
    	[$medianame,
    	sprintf("$medianame (%.2f in x %.2f in, %.1f GSM, %s, %s, %d)",
    		$width/72.0, $height/72.0, $weight, $colour, $type, $flag_suitability)
    	]);
    }
print "</pre>\n";
close(M) || die;

# Connect to ppop.
my $control = new PPR::PPOP($name);

# Let ppop know who we are.
if(undef_to_empty($ENV{REMOTE_USER}) ne "")
    {
    $control->su($ENV{REMOTE_USER});
    }

# Make any required changes.
print "<pre>\n";
foreach my $b (grep(/^bin_/, keys %data))
    {
    my $new = cgi_data_move($b, undef);
    my $old = cgi_data_move("_$b", undef);
    if($old ne $new)
    	{
	$b =~ /^bin_(.+)$/;
	my $bin = $1;
	my @result = $control->mount($bin, $new);
	if(@result)
	    {
	    print "Error while mounting \"$new\" on \"$bin\" instead of \"$old\":\n";
	    foreach my $line (@result)
	    	{
		print html($line), "\n";
	    	}
	    }
    	}
    }
print "</pre>\n";

print "<table>\n";
print "<tr><th>Bin</th><th>Medium: (Size Weight, Colour, Type, Banner Suitability)</th></tr>\n";

my @binlist = $control->media();
foreach my $i (@binlist)
    {
    my(undef, $bin, $medium) = @{$i};
    $data{"_bin_$bin"} = $medium;
    print "<tr>\n";
    print "<td>", html($bin), "</td>";
    print "<td><select name=", html_value("bin_$bin"), ">\n";
    print "<option value=\"\">\n";
    foreach my $i (@available)
	{
	my($mname, $mdescription) = @{$i};
	print "<option value=", html_value($mname);
	print " selected" if($mname eq $medium);
	print ">", html($mdescription), "\n";
	}
    print "</select></td>\n";
    print "</tr>\n";
    }

print "</table>\n";

print "<p class=\"buttons\">\n";
isubmit("action", "Close", N_("_Close"), 'class="buttons" onclick="window.close()"');
isubmit("action", "Apply", N_("_Apply"), 'class="buttons"');
print "</p>\n";

# This is the end of the exception handling block.  If die() was called
# within the block, print its message.
}; if($@)
	{
	my $message = html($@);
	print "<p>$message</p>\n";
	}

&cgi_write_data();

print <<"Tail";
</form>
</body>
</html>
Tail

exit 0;

