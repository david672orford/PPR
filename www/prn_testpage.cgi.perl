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
# Last modified 8 March 2002.
#

use lib "?";
require 'paths.ph';
require 'cgi_data.pl';
require 'cgi_wizard.pl';
require "cgi_intl.pl";
require 'cgi_run.pl';

#===========================================
# This is the table for this wizard:
#===========================================
$addprn_wizard_table = [
	#===========================================
	# Welcome
	#===========================================
	{
	'title' => N_("PPR Print Test Page"),
	'picture' => "wiz-newprn.jpg",
	'dopage' => sub {
		print "<p>", H_("Right now the test page is pretty poor.  It is just a blank page."), "</p>\n";
		},
	'buttons' => [N_("_Cancel"), N_("_Print")]
	},
	#===========================================
	# Do It
	#===========================================
	{
	'title' => N_("PPR Print Test Page"),
	'picture' => "wiz-newprn.jpg",
	'dopage' => sub {
		my $name = cgi_data_peek("name", "_missing_");
		print "<pre>\n";
		run("$HOMEDIR/bin/ppr-testpage | $HOMEDIR/bin/ppr -d $name");
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
