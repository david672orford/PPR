#! /usr/bin/perl -w
#
# mouse:~ppr/src/www/cliconf.cgi.perl
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
# Last modified 21 May 2002.
#

use lib "?";
require 'paths.ph';
require 'cgi_data.pl';
require 'cgi_wizard.pl';
require "cgi_intl.pl";

$printcap_wizard_table = [
	#===========================================
	# Choose a Spooler
	#===========================================
	{
	'title' => N_("Create Client Script: Choose a Spooler"),
	'picture' => "cliconf1.png",
	'dopage' => sub {
		my $name = cgi_data_peek("name", "?");
		$data{host} = $ENV{SERVER_NAME};

		print "<p>", html(sprintf(_("This program will create a script which you can download and run\n"
			. "in order to set up your local spooler to print to the PPR\n"
			. "queue \"%s\"."), $name)), "</p>\n";

		print "<p>", H_("Please indicate which print spooler you are using:"), "<br>\n";
		print '<input type="radio" name="spooler" value="lpr">', H_("BSD lpd"), "<br>\n";
		print '<input type="radio" name="spooler" value="lprng">', H_("LPR Next Generation"), "<br>\n";
		print "</p>\n";
		},
	'onnext' => sub {
		my $spooler = cgi_data_peek("spooler", undef);
		if(!defined($spooler))
		    {
		    return _("You must choose a spooler!");
		    }
		return undef;
		},
	'buttons' => [N_("_Cancel"), N_("_Next")]
	},

	#===========================================
	# Local Name
	#===========================================
	{
	'title' => N_("Create Client Script: Local Name"),
	'picture' => "cliconf1.png",
	'dopage' => sub {
		my $name = cgi_data_peek("name", "?");
		my $host = cgi_data_peek("host", "?");
		my $localname = cgi_data_move("localname", $name);
		my $comment = cgi_data_move("comment", "");

		print "<p>", html(sprintf(_("Please choose a local name for %s:"), "$name\@$host")), "<br>\n";
		print '<input type="text" size=16 name="localname" value=', html_value($localname), ">\n";
		print "</p>\n";

		print "<p>", H_("Please provide a description of this queue:"), "<br>\n";
		print '<input type="text" size=50 name="comment" value=', html_value($comment), ">\n";
		print "</p>\n";
		},
	'onnext' => sub {
		if(cgi_data_peek("localname", "") eq "")
		    {
		    return _("The local name must not be blank.");
		    }
		if(cgi_data_peek("comment", "") eq "")
		    {
		    return _("The description must not be blank.");
		    }
		return undef;
		}	
	},
	#===========================================
	# Download
	#===========================================
	{
	'title' => N_("Create Client Script: Download"),
	'picture' => "cliconf1.png",
	'dopage' => sub {
		my $localname = cgi_data_peek("localname", "?");
		print "<p>", H_("Click on the button below to download the install script."), "</p>\n";
		isubmit("action", "Download", N_("_Download"), undef), 
		print "<p>", H_("This script must be run as root.  The command to run it is:"), "</p>\n";
		print "<pre>\n";
		print html("# sh setup_$localname.sh"), "\n";
		print "</pre>\n";
		},
	'buttons' => [N_("_Close")]
	}
	

];

#===========================================
# Main
#===========================================

&cgi_read_data();

if(cgi_data_move("action", "") ne "Download")
    {
    &do_wizard($printcap_wizard_table,
	{
	'auth' => 0,
	'imgdir' => "../images/"
	});
    exit 0;
    }

my $spooler = cgi_data_move("spooler", "?");
my $localname = cgi_data_move("localname", "?");
my $name = cgi_data_move("name", "?");
my $host = cgi_data_move("host", "?");
my $comment = cgi_data_move("comment", "");
my $script_filename = "setup_$localname.sh";
my $creation_date = scalar localtime(time);

print <<"EndHead";
Content-Type: application/octet-stream; name=$script_filename
Content-Disposition: inline; filename=$script_filename

#! /bin/sh
#
# This script was generated by the PPR web interface.
# Created $creation_date.
#
# Spooler: $spooler
# Local name: $localname
# Remote name: $name
# Remote system: $host
# Comment: $comment
#

EndHead

if($spooler eq "lpr" || $spooler eq "lprng")
{
print <<"EndOfBSD";
cat >>/etc/printcap <<'EndOfPrintcap'

$localname|$comment:\\
	:lp=:\\
	:rm=$host:rp=$name:\\
	:sd=/var/spool/lpd/$name:\\
	:mx#0:\\
	:sh:

EndOfPrintcap

mkdir /var/spool/lpd/$name

EndOfBSD
}
else
{
die;
}

# end of file
