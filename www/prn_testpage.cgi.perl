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
# Last modified 5 April 2002.
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
		print "<p>", H_("To print a PPR test page, select the desired page size below\n"
			. "and press Print."), "</p>\n";

		my $pagesize = cgi_data_move("pagesize", "Letter");
		labeled_select("pagesize", _("Page Size:"), "Letter", $pagesize, qw(Letter A4));

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
		my $pagesize = cgi_data_peek("pagesize", "Letter");
		$pagesize =~ /^[a-zA-Z0-9]+$/ || die "$pagesize is not a valid pagesize";
		print "<pre>\n";
		run("$HOMEDIR/bin/ppr-testpage --pagesize=$pagesize | $HOMEDIR/bin/ppr -d $name");
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
	'imgdir' => "../images/"
	});

exit 0;
