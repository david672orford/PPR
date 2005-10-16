#! @PERL_PATH@ -w
#
# mouse:~ppr/src/www/cliconf.cgi.perl
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
# Last modified 15 October 2005.
#

use lib "@PERL_LIBDIR@";
require 'paths.ph';
require 'cgi_data.pl';
require 'cgi_wizard.pl';
require "cgi_intl.pl";

defined($SHAREDIR) || die;

$printcap_wizard_table = [
	#===========================================
	# Choose What to Create
	#===========================================
	{
	'title' => N_("Downloadable Client Configuation Files"),
	'picture' => "cliconf1.png",
	'dopage' => sub {
			my $name = cgi_data_peek("name", "?");

			if(!defined $data{comment})
				{
				$data{comment} = (split(/\t/, `$PPOP_PATH -M ldest $name`))[4];
				}

			print "<p>", 
					H_("Using this program you can download various files which help you to use PPR.\n"
					. "These include shell scripts for configuring client computers to send print\n"
					. "jobs to PPR, PPD files, and icons for opening the PPR web interface."), "</p>\n";

			print "<p><span class=\"label\">", sprintf(_("What do you want to download for the queue \"%s\"?"), $name), "</span><br>\n";

			print '<label><input type="radio" name="what_to_download" value="spooler_config">', H_("Spooler Configuration Script"), "</label><br>\n";
			print '<label><input type="radio" name="what_to_download" value="icon_dotdesktop_web">', H_("KDE/Gnome Shortcut to Web Interface"), "</label><br>\n";
			print '<label><input type="radio" name="what_to_download" value="icon_dotdesktop_perltk">', H_("KDE/GNOME Shortcut to Perl/Tk Interface"), "</label><br>\n";
			#print '<input type="radio" name="what_to_download" value="mswin_shortcut">', H_("MS-Windows Shortcut"), "<br>\n";
			#print '<input type="radio" name="what_to_download" value="ppd">', H_("PPD File"), "<br>\n";
			},
	'buttons' => [N_("_Cancel"), N_("_Next")],
	'onnext' => sub {
		if(! defined(cgi_data_peek("what_to_download", undef)))
			{
			return _("You must make a choice.");
			}
		return undef;
		},
	'getnext' => sub {
		return cgi_data_peek("what_to_download", "");
		}
	},

	#===========================================
	# KDE Icon for Web Interface
	#===========================================
	{
	'label' => 'icon_dotdesktop_web',
	'title' => N_("Downloadable KDE/Gnome Icon"),
	'picture' => "cliconf1.png",
	'dopage' => sub {
		print "<p>",
			H_("Click on the button below to download the KDE/Gnome icon for the PPR Web\n"
			 . "Interface.  You will probably want to save it in your Desktop folder."),
			"</p>\n";
		isubmit("action", "Download", N_("_Download"));
		},
	'buttons' => [N_("_Close")]
	},

	#===========================================
	# KDE Icon for Perl/Tk Interface
	#===========================================
	{
	'label' => 'icon_dotdesktop_perltk',
	'title' => N_("Downloadable KDE/Gnome Icon"),
	'picture' => "cliconf1.png",
	'dopage' => sub {
		print "<p>",
			H_("Click on the button below to download the KDE/Gnome icon for the PPR Perl/Tk\n"
			 . "Interface.  You will probably want to save it in your Desktop folder."),
			"</p>\n";
		isumit("action", "Download", N_("_Download"));
		},
	'buttons' => [N_("_Close")]
	},

	#===========================================
	# Choose a Spooler
	#===========================================
	{
	'label' => "spooler_config",
	'title' => N_("Create Client Script: Choose a Spooler"),
	'picture' => "cliconf1.png",
	'dopage' => sub {
		die unless(cgi_data_peek("what_to_download", "?") eq "spooler_config");

		my $name = cgi_data_peek("name", "?");
		$data{host} = $ENV{SERVER_NAME};

		print "<p>", html(sprintf(
				_("You have chosen to create a script which you can download and run\n"
				. "in order to set up your local spooler to print to the PPR\n"
				. "queue \"%s\"."), $name)), "</p>\n";

		print "<p><span class=\"label\">", H_("Please indicate which print spooler you are using:"), "</span><br>\n";
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
		}
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
		isubmit("action", "Download", N_("_Download")); 
		print "<p>", H_("This script must be run as root.  The command to run it is:"), "</p>\n";
		print "<pre>\n";
		print html("# ./setup_$localname.sh"), "\n";
		print "</pre>\n";
		},
	'buttons' => [N_("_Close")]
	}
];

#===========================================
# This function answers a request to 
# download a spooler configuration script.	
#===========================================

sub gen_spooler_config
	{
	my $spooler = cgi_data_move("spooler", "?");
	my $localname = cgi_data_move("localname", "?");
	my $name = cgi_data_move("name", "?");
	my $host = cgi_data_move("host", "?");
	my $comment = cgi_data_move("comment", "");
	my $script_filename = "setup_$localname.sh";
	my $creation_date = scalar localtime(time);

	print <<"EndHead";
Content-Type: application/octet-stream; name=$script_filename
Content-Disposition: attachment; filename=$script_filename

#! @SHELL@
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
	}

#====================================================
# This function creates a Freedesktop.org shortcut
#====================================================
sub gen_dotdesktop_icon
	{
	my $prog = shift;
	my $name = cgi_data_move("name", "?");
	my $filename = "$name.desktop";

	my $comment = cgi_data_move("comment", "");

	print <<"EndIcon";
Content-Type: application/x-desktop; name=$filename
Content-Disposition: attachment; filename=$filename

[Desktop Entry]
Encoding=UTF-8
Version=1.0
Type=Application
Exec=$prog -d $name
TryExec=
Icon=$SHAREDIR/www/images/icon-48.xpm
Terminal=false
Name=$name
GenericName=
Comment=$comment
EndIcon
	}

#===========================================
# Main
#===========================================

&cgi_read_data();

if(cgi_data_move("action", "") eq "Download")
	{
	my $what = cgi_data_peek("what_to_download", "");
	if($what eq "spooler_config")
		{
		gen_spooler_config();
		}
	elsif($what eq "icon_dotdesktop_web")
		{
		gen_dotdesktop_icon("ppr-web");
		}
	elsif($what eq "icon_dotdesktop_perltk")
		{
		gen_dotdesktop_icon("ppr-panel");
		}
	else
		{
		die;
		}
	}
else
	{
	&do_wizard($printcap_wizard_table,
		{
		'auth' => 0,
		'imgdir' => "../images/"
		});
	exit 0;
	}

# end of file
