#! /usr/bin/perl -w
#
# mouse:~ppr/src/www/prn_testpage.cgi.perl
# Copyright 1995--2004, Trinity College Computing Center.
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
# Last modified 27 May 2004.
#

use lib "?";
require "paths.ph";
require "cgi_data.pl";
require "cgi_wizard.pl";
require "cgi_widgets.pl";
require "cgi_intl.pl";
require "cgi_run.pl";

# Where are the Ghostscript example PostScript files?  This is the
# location of the PPR Ghostscript distribution.
$GS_EXAMPLES = "$SHAREDIR/../ppr-gs/examples";
%IMAGES = (
	"Ghostscript Golfer" =>
		["$GS_EXAMPLES/golfer.eps", 0.25],
	"Ghostscript Tiger" => 
		["$GS_EXAMPLES/tiger.eps", 0.25]
	"Ghostscript Color Circle" => 
		["$GS_EXAMPLES/colorcir.ps", 0.25]
	"Ghostscript Escher" => 
		["$GS_EXAMPLES/escher.ps", 0.25]
	"Ghostscript Dore Tree" => 
		["$GS_EXAMPLES/doretree.ps", 0.25]
	);

# Eliminate those that aren't installed.
foreach my $i (keys %IMAGES)
		{
		if(! -f $IMAGES{$i}->[0])
			{
			delete $IMAGES{$i};
			}
		}

#===========================================
# This is the table for this wizard:
#===========================================
$addprn_wizard_table = [
	#===========================================
	# Welcome
	#===========================================
	{
	'title' => N_("PPR Print Test Page"),
	'picture' => "prn_testpage1.png",
	'dopage' => sub {
		print "<p>", H_("To print a PPR test page, select the desired options below\n"
				. "and press [Print]."), "</p>\n";

		print "<p>\n";
		my $default_pagesize = "Letter";
		my $pagesize = cgi_data_move("pagesize", $default_pagesize);
		labeled_select("pagesize", _("Page Size:"), $default_pagesize, $pagesize,
				qw(Letter Legal A4));
		print "</p>\n";

		print "<p>\n";
		my $default_image = "PPR Logo";
		my $image = cgi_data_move("image", $default_image);
		labeled_select("image", _("Image:"), $default_image, $image, ("PPR Logo", keys(%IMAGES)));
		print "</p>\n";

		print "<div class=\"section\">\n";
		print "<span class=\"section_label\">", H_("Color Space Test Patterns"), "</span>\n";
			print "<p>\n";
			labeled_boolean("test_grayscale", _("Grayscale"), cgi_data_move("test_grayscale", 0));
			print "</p>\n";
			print "<p>\n";
			labeled_boolean("test_rgb", _("RGB"), cgi_data_move("test_rgb", 0));
			print "</p>\n";
			print "<p>\n";
			labeled_boolean("test_cmyk", _("CMYK"), cgi_data_move("test_cmyk", 0));
			print "</p>\n";
		print "</div>\n";

		print "<div class=\"section\">\n";
		print "<span class=\"section_label\">", H_("Other Test Patterns"), "</span>\n";
		print "<p>\n";
		labeled_boolean("test_spokes", _("Spoked Wheel"), cgi_data_move("test_spokes", 0));
		print "</p>\n";
		print "</div>\n";
		},
	'buttons' => [N_("_Cancel"), N_("_Print")]
	},
	
	#===========================================
	#
	#===========================================
	{
	'title' => N_("PPR Print Test Page"),
	'picture' => "prn_testpage1.png",
	'dopage' => sub {
		}
	},

	#===========================================
	# Do It
	#===========================================
	{
	'title' => N_("PPR Print Test Page"),
	'picture' => "prn_testpage2.png",
	'dopage' => sub {
		my $name = cgi_data_peek("name", "_missing_");

		my @ppr_testpage = ("$HOMEDIR/bin/ppr-testpage");
		my @ppr = ("$HOMEDIR/bin/ppr", "-d", $name,
			"-m", "pprpopup",
			"-r", "$ENV{REMOTE_USER}\@$ENV{REMOTE_ADDR}"
			);

		my $pagesize = cgi_data_move("pagesize", "Letter");
		$pagesize =~ /^[a-zA-Z0-9]+$/ || die "$pagesize is not a valid pagesize";
		push(@ppr_testpage, "--pagesize=$pagesize");

		my $image = cgi_data_move("image", "");
		if($image ne "PPR Logo")
			{
			push(@ppr_testpage, "--eps-file=$IMAGES{$image}->[0]",
				"--eps-scale=$IMAGES{$image}->[1]"
				);
			}

		if(cgi_data_move("test_grayscale", 0))
			{
			push(@ppr_testpage, "--test-grayscale");
			}
		if(cgi_data_move("test_rgb", 0))
			{
			push(@ppr_testpage, "--test-rgb");
			}
		if(cgi_data_move("test_cmyk", 0))
			{
			push(@ppr_testpage, "--test-cmyk");
			}
		if(cgi_data_move("test_spokes", 0))
			{
			push(@ppr_testpage, "--test-spokes", "--eps-scale=0.75");
			}

		print "<pre>\n";
		run_pipeline(\@ppr_testpage, \@ppr);
		print "</pre>\n";
		},
	'buttons' => [N_("_Back"), N_("_Close")]
	}
];

#===========================================
# Main
#===========================================

&cgi_read_data();

if(cgi_data_peek("wiz_action", "") eq "Print")
	{
	$data{wiz_action} = "Finish";
	}

&do_wizard($addprn_wizard_table,
		{
		'auth' => 1,
		'imgdir' => "../images/",
		'debug' => 0
		});

exit 0;
