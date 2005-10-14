#! @PERL_PATH@ -wT
#
# mouse:~ppr/src/www/prn_media.cgi.perl
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
# Last modified 14 October 2005.
#

use lib "@PERL_LIBDIR@";
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

if(undef_to_empty($ENV{REMOTE_USER}) eq "" && $action ne "")
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
<link rel="stylesheet" href="../style/shared.css" type="text/css">
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
close(M) || die;

# Get the name of the PPD file
if(!defined($data{ppd}))
	{
	open(CONF, "<$PPR::PRCONF/$name") || die $!;
	while(<CONF>)
		{
		if(/^PPDFile: (.+)$/)
			{
			$data{ppd} = $1;
			last;
			}
		}
	close(CONF) || die $!;
	}

# Get the bin translations.
if(!defined($data{ppd_bins}))
	{
	require "readppd.pl";
	ppd_open($data{ppd});
	while(defined($line = ppd_readline()))
		{
		if($line =~ /^\*InputSlot\s+([^\/:]+)(\/([^:]+))?/)
			{
			my ($name, $translation) = ($1, $3);
			$data{"xlate_$name"} = $translation;
			}
		}
	$data{ppd_bins} = 1;
	}

# Connect to ppop.
my $control = new PPR::PPOP($name);

# Let ppop know who we are.
$control->user("$ENV{REMOTE_USER}\@$ENV{REMOTE_ADDR}");

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

# This table will list the medium mounted on each bin in a select control so
# that it can be changed.
print "<table cellspacing=0>\n";
print "<tr><th>Bin</th><th>Medium: (Size Weight, Colour, Type, Banner Suitability)</th></tr>\n";
print "<tbody>\n";

my @binlist = $control->media();
foreach my $i (@binlist)
	{
	my(undef, $bin, $medium) = @{$i};
	$data{"_bin_$bin"} = $medium;
	print "<tr>\n";

	my $bin_description = html($bin);
	if(defined($data{"xlate_$bin"}))
		{
		$bin_description = $bin . " -- " . $data{"xlate_$bin"};
		}
	$bin_description = html_nb($bin_description);
	$bin_description =~ s/--/&#151;/g;

	print "<th>", $bin_description, "</th>";
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

print "</tbody>\n";
print "</table>\n";

print "<p class=\"buttons\">\n";
isubmit("action", "Close", N_("_Close"), _("Close this window."), "window.close()");
isubmit("action", "Apply", N_("_Apply"), _("Save changes."));
print "</p>\n";

# This is the end of the exception handling block.	If die() was called
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

