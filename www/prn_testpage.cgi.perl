#! /usr/bin/perl -w
#
# mouse:~ppr/src/www/prn_testpage.cgi.perl
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
# Last modified 17 April 2002.
#

use lib "?";
require "paths.ph";
require "cgi_data.pl";
require "cgi_wizard.pl";
require "cgi_widgets.pl";
require "cgi_intl.pl";
require "cgi_run.pl";

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
		labeled_select("image", _("Image:"), $default_image, $image, ("PPR Logo", "Ghostscript Golfer", "Ghostscript Tiger"));
		print "</p>\n";

		print "<div class=\"section\">\n";
		print "<span class=\"section\">", H_("Color Space Test Patterns"), "</span>\n";
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
		},
	'buttons' => [N_("_Cancel"), N_("_Print")]
	},
	#===========================================
	# Do It
	#===========================================
	{
	'title' => N_("PPR Print Test Page"),
	'picture' => "prn_testpage2.png",
	'dopage' => sub {
		my $name = cgi_data_peek("name", "_missing_");

		my $options = "";

		my $pagesize = cgi_data_move("pagesize", "Letter");
		$pagesize =~ /^[a-zA-Z0-9]+$/ || die "$pagesize is not a valid pagesize";
		$options .= " --pagesize=$pagesize";

		my $image = cgi_data_move("image", "");
		if($image eq "Ghostscript Golfer")
		    {
		    $options .= " --eps-file=$SHAREDIR/gs/golfer.ps --eps-scale=0.40";
		    }
		elsif($image eq "Ghostscript Tiger")
		    {
		    $options .= " --eps-file=$SHAREDIR/gs/tiger.ps --eps-scale=0.40";
		    }

		if(cgi_data_move("test_grayscale", 0))
		    {
		    $options .= " --test-grayscale";
		    }
		if(cgi_data_move("test_rgb", 0))
		    {
		    $options .= " --test-rgb";
		    }
		if(cgi_data_move("test_cmyk", 0))
		    {
		    $options .= " --test-cmyk";
		    }

		run("$HOMEDIR/bin/ppr-testpage$options | $HOMEDIR/bin/ppr -d $name");
		print "</pre>\n";
		},
	'buttons' => [N_("_Close")]
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
