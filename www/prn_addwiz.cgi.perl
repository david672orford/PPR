#! /usr/bin/perl -wT
#
# mouse:~ppr/src/misc/prn_addwiz.cgi.perl
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
# Last modified 16 April 2004.
#

#
# This CGI script is a `Wizard', one of those programs which helps someone
# set something up by breaking the problem down into a series of questions
# which are presented on a chained set of pages.
#
# The purpose of this wizard is to set up a printer queue.
#

use lib "?";
require 'paths.ph';
require 'cgi_data.pl';
require 'cgi_wizard.pl';
require "cgi_intl.pl";
require 'cgi_widgets.pl';

defined($HOMEDIR) || die;
defined($INTDIR) || die;
defined($CONFDIR) || die;
defined($PPR2SAMBA_PATH) || die;

#======================================================
# This is a table of interface program descriptions.
#======================================================
%interface_descriptions = (
		'dummy'			=> "--hide",
		'simple'		=> N_("Server Generic Port"),
		'serial'		=> N_("Server Serial Port"),
		'parallel'		=> N_("Server Parallel Port"),
		'usblp'			=> N_("Server USB Port"),
		'atalk'			=> N_("Network via AppleTalk PAP"),
		'tcpip'			=> N_("Network via raw TCP"),
		'socketapi'		=> N_("Network via SocketAPI"),
		'appsocket'		=> N_("Network via AppSocket"),
		'jetdirect'		=> N_("Network via HP JetDirect"),
		'pros'			=> N_("Network via AXIS PROS"),
		'lpr'			=> N_("Network via LPD"),
		'smb'			=> N_("Network via LAN Manager/MS-Windows"),
		'ppromatic'		=> "--hide",
		'foomatic-rip'	=> "--hide",
		'gssimple'		=> "--hide",
		'gsatalk'		=> "--hide",
		'gstcpip'		=> "--hide",
		'gsserial'		=> "--hide",
		'gsparallel'	=> "--hide",
		'gslpr'			=> "--hide",
		'gssmb'			=> "--hide"
		);

# This table indentifies interfaces which have custom 
# manual configuration pages.
%interface_pages = (
	"tcpip"		=> "int_tcpip",
	"socketapi"	=> "int_tcpip",
	"appsocket"	=> "int_tcpip",
	"jetdirect"	=> "int_jetdirect",
	"lpr" => "int_lpr",
	"pros" => "int_pros"
	);

#===========================================
# This is the table for this wizard:
#===========================================
$addprn_wizard_table = [
		#===========================================
		# Welcome
		#===========================================
		{
		'title' => N_("PPR: Add a Printer"),
		'picture' => "wiz-newprn.jpg",
		'dopage' => sub {
				print "<p><span class=\"label\">", H_("PPR Printer Queue Creation"), "</span></p>\n";
				print "<p>", H_("This program will guide you through the process of setting up a printer in PPR."), "</p>\n";

				my $method = cgi_data_move("method", "browse_printers");
				labeled_radio("method", "Search for printers", "browse_printers", $method);
				print "<br>\n";
				labeled_radio("method", "Manually configure printer", "choose_int", $method);
				},
		'onnext' => sub {
				if(! defined($data{method}))
					{ return _("You must make a selection!") }
				return undef;
				},
		'getnext' => sub {
				return $data{method};
				},
		'buttons' => [N_("_Cancel"), N_("_Next")]
		},

		#===========================================
		# Choose an interface
		#===========================================
		{
		'label' => 'choose_int',
		'title' => N_("PPR: Add a Printer: Select an Interface Program"),
		'picture' => "wiz-interface.jpg",
		'dopage' => sub {
				# Get a sorted list of the available interfaces.
				opendir(I, $INTDIR) || die "opendir() failed on \"$INTDIR\", $!";
				my @interfaces = ();
				foreach my $i (sort(grep(!/^\./, readdir(I))))
					{
					my $desc = $interface_descriptions{$i};
					push(@interfaces, $i) if(!defined $desc || $desc ne "--hide");
					}
				closedir(I);

				print "<p>", H_("Please select an interface program.  PPR uses an interface\n"
						. "program to communicate with the printer.  There is an\n"
						. "interface program for each communication method.  The\n"
						. "interfaces whose names begin with \"gs\" pass PostScript jobs\n"
						. "through Ghostscript before sending them to the printer."), "</p>\n";

				print "<p><span class=\"label\">", H_("Interface Program:"), "</span><br>\n";

				print "<table><tr align=\"left\" valign=\"top\"><td>\n";
				my $interface;
				my $checked_interface = cgi_data_move('interface', '');
				my $thiscol = 0;
				my $percol = int(scalar(@interfaces) / 2 + 0.5);
				print STDERR "\$percol=$percol\n";
				while(defined($interface = shift @interfaces))
					{
					if(++$thiscol > $percol)
						{
						print "</td><td>\n";
						$thiscol = 0;
						}
					my $desc = $interface_descriptions{$interface};
					print "<label><input tabindex=1 TYPE=\"radio\" NAME=\"interface\" VALUE=", html_value($interface);
					print " checked" if($interface eq $checked_interface);
					print "> $interface";
					print " - <font size=-3>", H_($desc), "</font>" if(defined($desc));
					print "</label><br>\n";
					}
				print "</td></tr></table>\n";
				},
		'onnext' => sub {
				if(! defined($data{interface}))
					{ return _("You must make a selection!") }
				return undef;
				},
		'getnext' => sub {
				my $interface = $data{interface};

				# Where do we go next?
				if(defined(my $goto = $interface_pages{$interface}))
					{
					return $goto;
					}
				return 'int_generic';
				}
		},

		#===============================================
		# Interface setup for TCP/IP socket protocols
		#===============================================
		{
		'label' => 'int_tcpip',
		'title' => N_("PPR: Add a Printer: Raw TCP/IP Interface: Choose an Address"),
		'picture' => "wiz-address.jpg",
		'dopage' => sub {
				print "<p>", H_("It is necessary to know the IP address of the printer,\n"
								. "or preferably a DNS name which represents that address.\n"
								. "An example of a DNS name is \"myprn.prn.myorg.org\".  An example\n"
								. "of an IP address is \"157.252.200.22\"."), "</p>\n";

				print "<p>", H_("DNS Name or IP Address:"), " ";
				print "<input tabindex=1 name=\"int_tcpip_name\" size=32 value=", html_value(cgi_data_move('int_tcpip_name', '')), ">\n";
				print "</p>\n";

				print "<p>", H_("HP JetDirect cards accept jobs on TCP port 9100.\n"
								. "For AXIS print servers it is 9900.  For other brands,\n"
								. "consult the product manual."), "</p>\n";

				print "<p>", H_("TCP Port Number:"), " ";
				print "<input tabindex=1 name=\"int_tcpip_port\" size=8 value=", html_value(cgi_data_move('int_tcpip_port', '9100')), ">\n";
				print "</p>\n";

				print "<p>",
					H_("Bidirectional communication is highly desirable, however many devices don't\n"
					. "support it.  If you say Yes here and the correct answer is No, the queue will\n"
					. "get stuck at the end of the first job."), "</p>\n";

				print "<p>", H_("Bidirectional:"), " ";
				print "<select tabindex=1 name=\"int_tcpip_feedback\">\n";
				my $feedback = cgi_data_move('int_tcpip_feedback', 'No');
				print "<option";
				print " selected" if($feedback eq 'Yes');
				print ">Yes\n";
				print "<option";
				print " selected" if($feedback eq 'No');
				print ">No\n";
				print "</select>\n</p>\n";
				},
		'onnext' => sub {
				if($data{'int_tcpip_name'} eq '')
					{ return _("You must fill in the printer's DNS name or IP address!") }
				if($data{'int_tcpip_port'} eq '')
					{ return _("You must fill in the TCP port number!") }
				$data{address} = $data{int_tcpip_name} . ':' . $data{int_tcpip_port};
				$data{options} = "";
				$data{jobbreak} = "default";
				$data{feedback} = $data{int_tcpip_feedback};
				$data{codes} = "default";

				return undef;
				},
		'getnext' => sub { return 'ppd' }
		},

		#===========================================
		# Interface setup for HP JetDirect
		#===========================================
		{
		'label' => 'int_jetdirect',
		'title' => N_("PPR: Add a Printer: JetDirect Interface: Choose an Address"),
		'picture' => "wiz-address.jpg",
		'dopage' => sub {
				print "<p>", H_("It is necessary to know the IP address of the printer,\n"
								. "or preferably a DNS name which represents that address.\n"
								. "An example of a DNS name is \"myprn.prn.myorg.org\".  An example\n"
								. "of an IP address is \"157.252.200.22\"."), "</p>\n";

				print "<p>", H_("DNS Name or IP Address:"), " ";
				print "<input tabindex=1 name=\"int_jetdirect_name\" size=32 value=", html_value(cgi_data_move('int_jetdirect_name', '')), ">\n";
				print "</p>\n";

				print "<p>", H_("Most HP JetDirect cards accept jobs on TCP port 9100.\n"), "</p>\n";

				print "<p>", H_("TCP Port Number:"), " ";
				print "<input tabindex=1 name=\"int_jetdirect_port\" size=8 value=", html_value(cgi_data_move('int_jetdirect_port', '9100')), ">\n";
				print "</p>\n";

				print "<p>",
					H_("Internal JetDirect cards support bidirectional communication.  External print\n"
					. "servers will only support bidirectional communication if they communicate with\n"
					. "the printer through a serial port or through a bidirectional parallel port.  If\n"
					. "you say Yes here and the correct answer is No, the queue will get stuck at the\n"
					. "end of the first job."), "</p>\n";

				print "<p>", H_("Bidirectional:"), " ";
				print "<select tabindex=1 name=\"int_jetdirect_feedback\">\n";
				my $feedback = cgi_data_move('int_jetdirect_feedback', 'Yes');
				print "<option";
				print " selected" if($feedback eq 'Yes');
				print ">Yes\n";
				print "<option";
				print " selected" if($feedback eq 'No');
				print ">No\n";
				print "</select>\n</p>\n";
				},
		'onnext' => sub {
				if($data{'int_jetdirect_name'} eq '')
					{ return _("You must fill in the printer's DNS name or IP address!") }
				if($data{'int_jetdirect_port'} eq '')
					{ return _("You must fill in the TCP port number!") }
				$data{address} = $data{int_jetdirect_name} . ':' . $data{int_jetdirect_port};
				$data{options} = "";
				$data{jobbreak} = "default";
				$data{feedback} = $data{int_jetdirect_feedback};
				$data{codes} = "default";

				return undef;
				},
		'getnext' => sub { return 'ppd' }
		},

		#===========================================
		# Interface setup for RFC 1179 (LPR/LPD)
		#===========================================
		{
		'label' => 'int_lpr',
		'title' => N_("PPR: Add a Printer: LPR Interface: Choose an Address"),
		'picture' => "wiz-address.jpg",
		'dopage' => sub {
				print "<p>",
						H_("PPR needs to know the name or IP address of the printer or of the print server\n"
						. "to which it is attached.  An example of a DNS name is \"myhost.myorg.org\".  An\n"
						. "example of an IP address is \"157.252.200.52\".  A name is preferred."), "</p>\n";

				print "<p>";
				labeled_entry("int_lpr_host", _("DNS Name or IP Address:"), cgi_data_move("int_lpr_host", ""), 32);
				print "</p>\n";

				print "<p>";
				print H_("What is the name of the printer on the remote print server?\n");
				print H_("See the table below.");
				print "</p>\n";

				print "<p>";
				labeled_entry("int_lpr_printer", _("Remote Queue Name:"), cgi_data_move('int_lpr_printer', ""), 16);
				print "</p>\n";

				print "<table class=\"lines\" cellspacing=0>\n";
				print "<tr><th>", H_("Brand"), "</th><th>", H_("Product"), "</th><th>", H_("Remote Queue Name"), "</th></tr>\n";
				print "<tr><td>HP</td>					<td>Jetdirect</td>			<td>raw</td></tr>\n";
				print "<tr><td>Ricoh</td>				<td>Aficio</td>				<td>PORT1</td></tr>\n";
				print "<tr><td>Extended Systems</td>	<td>PocketPrintServer</td>	<td>PPSQueue</td></tr>\n";
				print "<tr><td>AXIS Communications</td>	<td>540+/542+</td>			<td>LPT1</td></tr>\n";
				print "</table>\n";
				},
		'onnext' => sub {
				if($data{int_lpr_host} eq '')
					{ return _("You must fill in the remote host's DNS name or IP address!") }
				if($data{int_lpr_printer} eq '')
					{ return _("You must fill in the remote queue name!") }
				$data{address} = $data{int_lpr_printer} . '@' . $data{int_lpr_host};
				$data{options} = "";
				$data{jobbreak} = "default";
				$data{feedback} = "default";
				$data{codes} = "default";
				return undef;
				},
		'getnext' => sub { return 'ppd' }
		},

		#===========================================
		# Interface setup for PROS
		#===========================================
		{
		'label' => 'int_pros',
		'title' => N_("PPR: Add a Printer: LPR Interface: Choose an Address"),
		'picture' => "wiz-address.jpg",
		'dopage' => sub {
				print "<p>",
						H_("PPR needs to know the name or IP address of the printer or of the print server\n"
						. "to which it is attached.  An example of a DNS name is \"myhost.myorg.org\".  An\n"
						. "example of an IP address is \"157.252.200.52\".  A name is preferred."), "</p>\n";

				print "<p>";
				labeled_entry("int_pros_host", _("DNS Name or IP Address:"), cgi_data_move("int_pros_host", ""), 32);
				print "</p>\n";

				print "<p>", H_("What is the name of the printer on the remote print server?"), "</p>\n";

				print "<p>";
				labeled_entry("int_pros_printer", _("Remote Queue Name:"), cgi_data_move('int_pros_printer', "LPT1"), 16);
				print "</p>\n";
				},
		'onnext' => sub {
				if($data{int_pros_host} eq '')
					{ return _("You must fill in the remote host's DNS name or IP address!") }
				if($data{int_pros_printer} eq '')
					{ return _("You must fill in the remote queue name!") }
				$data{address} = $data{int_pros_printer} . '@' . $data{int_pros_host};
				$data{options} = "";
				$data{jobbreak} = "default";
				$data{feedback} = "default";
				$data{codes} = "default";
				return undef;
				},
		'getnext' => sub { return 'ppd' }
		},

		#===========================================
		# Interface setup for unrecognized
		# interfaces
		#===========================================
		{
		'label' => 'int_generic',
		'title' => N_("PPR: Add a Printer: Interface Setup"),
		'picture' => "wiz-address.jpg",
		'dopage' => sub {
				print "<p>", html(sprintf(_("No special help is available for the interface called\n"
						. "\"%s\".  Therefore, we present the generic\n"
						. "interface configuration form below.  You may have to refer to the\n"
						. "ppad(1) man page to determine the correct format\n"
						. "for the values of the fields below."), $data{interface})), "</p>\n";

				print "<p><span class=\"label\">", H_("Enter Printer Address:"), "</span><br>\n";
				print "<input tabindex=1 TYPE=\"text\" SIZE=40 NAME=\"int_generic_address\" VALUE=", html_value(cgi_data_move('int_generic_address', '')), ">\n";
				print "</P>\n";

				print "<p><span class=\"label\">", H_("Enter Interface Options:"), "</span><br>\n";
				print "<input tabindex=1 TYPE=\"text\" SIZE=40 NAME=\"int_generic_options\" VALUE=", html_value(cgi_data_move('int_generic_options', '')), ">\n";
				print "</P>\n";

				print "<p><span class=\"label\">", H_("Do the Interface and Printer Support 2-Way Communication?"), "</span><br>\n";
				my $checked_feedback = cgi_data_move('int_generic_feedback', 'default');
				my $feedback;
				foreach $feedback (qw(yes no default))
					{
					print "<input tabindex=1 TYPE=\"radio\" NAME=\"int_generic_feedback\" VALUE=\"$feedback\"", ($feedback eq $checked_feedback) ? " CHECKED" : "", "> ";
					print H_($feedback), " ";
					}
				print "</p>\n";

				print "<p><span class=\"label\">", H_("Choose a Jobbreak Method:"), "</span><br>\n";
				my $checked_jobbreak = cgi_data_move('int_generic_jobbreak', 'default');
				my $jobbreak;
				print "<select tabindex=1 NAME=\"int_generic_jobbreak\">\n";
				foreach $jobbreak (qw(default control-d pjl signal signal/pjl newinterface))
					{
					print "<option", $jobbreak eq $checked_jobbreak ? " selected" : "", ">";
					print "$jobbreak\n";
					}
				print "</select>\n";
				print "</p>\n";
				},
		'onnext' => sub {
				if($data{int_generic_address} eq '')
					{ return _("You must enter an address for the printer!") }
				$data{address} = $data{int_generic_address};
				$data{options} = $data{int_generic_options};
				$data{jobbreak} = $data{int_generic_jobbreak};
				$data{feedback} = $data{int_generic_feedback};
				$data{codes} = "default";
				return undef;
				},
		'getnext' => sub {
				return 'ppd';
				}
		},

		#===========================================
		# Browse printers--select zone
		#===========================================
		{
		'label' => 'browse_printers',
		'title' => N_("PPR: Add a Printer: Browse Printers"),
		'picture' => "wiz-newprn.jpg",
		'dopage_returns_html' => 1,
		'dopage' => sub {
			require 'cgi_run.pl';
			my $browser_zone = cgi_data_move("browser_zone", "");
			opendir(BROWSERS, "$HOMEDIR/browsers") || die $!;
			print '<p><label>', H_("Zones Available for Browsing"), '<br>', "\n";
			print '<select tabindex=1 name="browser_zone" size="20" style="min-width: 450px; max-width: 450px;">', "\n";
			while(my $browser = readdir(BROWSERS))
				{
				next if($browser =~ /^\./);
				$browser =~ /^([a-z0-9_-]+)$/ || die "browser=$browser";
				$browser = $1;
				print "<optgroup label=", html_value($browser), ">\n";
				opencmd(ZONES, "$HOMEDIR/browsers/$browser") || die "Unable to get zone list";
				while(my $zone = <ZONES>)
					{
					chomp $zone;
					if($zone =~ /^;(.+)$/)
						{
						my $browser_comment = $1;
						print "<option value=\"\">", html($browser_comment), "</option>\n";
						next;
						}
					my $new_browser_zone = "$browser:$zone";
					print "<option value=", html_value($new_browser_zone);
					print " selected" if($new_browser_zone eq $browser_zone);
					print ">", html_nb($zone), "</option>\n";
					}
				close(ZONES) || die $!;
				print "</optgroup>\n";
				}
			print "</select>\n";
			print "</span></p>\n";
			closedir(BROWSERS) || die $!;
			return H_("Up to a minute may be required to search some zones.");
			},
		'onnext' => sub {
				if(! defined($data{browser_zone}) || $data{browser_zone} eq "")
					{ return _("You must choose a zone!") }
				return undef;
				}
		},
		
		#===========================================
		# Browse printers--select printer
		#===========================================
		{
		'title' => N_("PPR: Add a Printer: Browse Printers"),
		'picture' => "wiz-newprn.jpg",
		'dopage_returns_html' => 1,
		'dopage' => sub {
			require 'cgi_run.pl';
			my $browser_zone = cgi_data_peek("browser_zone", "");
			my $browser_printer = cgi_data_move("browser_printer", "");
			my($browser, $zone) = split(/:/, $browser_zone, 2);
			$browser =~ /^([a-z0-9_-]+)$/ || die "browser=$browser";
			$browser = $1;
			opencmd(PRINTERS, "$HOMEDIR/browsers/$browser", $zone) || die;
			print '<p><label>', html(sprintf(_("Printers Available in Zone %s %s"), $browser, $zone)), '<br>', "\n";
			print '<select tabindex=1 name="browser_printer" size="20" style="min-width: 450px; max-width: 550px;">', "\n";
			my @browser_comments = ();
			outer:
			while(1)
				{
				my $name = "";
				my $manufacturer = "";
				my $model = "";
				my $manufacturer_model = "";
				my @interfaces = ();
				while(my $line = <PRINTERS>)
					{
					chomp $line;
					#print STDERR "\"$line\"\n";
					if($line =~ /^;(.+)$/)
						{
						push(@browser_comments, $1);
						}
					elsif($line =~ /^\[([^\]]+)\]$/)
						{
						$name = $1;
						}
					elsif($line =~ /^interface=(.+)$/)
						{
						push(@interfaces, $1);
						}
					elsif($line =~ /^manufacturer=(.*)$/)
						{
						$manufacturer = $1;
						}
					elsif($line =~ /^model=(.*)$/)
						{
						$model = $1;
						}
					elsif($line =~ /^manufacturer-model=(.*)$/)
						{
						$manufacturer_model = $1;
						}
					elsif($line =~ /^$/)	# end of record
						{
						my $label = $name;
						if($manufacturer ne "" && $model ne "")
							{
							$label .= " ($manufacturer $model)";
							}
						elsif($manufacturer_model ne "")
							{
							$label .= " ($manufacturer_model)";
							}
						print "<optgroup label=", html_value($label), ">\n";
						foreach my $interface (@interfaces)
							{
							my($interface_name, $interface_address) = split(/,/, $interface, 2);
							my $interface_label = "$interface_name $interface_address";
							print "<option value=", html_value($interface), ">",
								html($interface_label),
								"</option>\n";
							}
						print "</optgroup>\n";
						next outer;
						}
					else
						{
						push(@browser_comments, "???: $line");
						}
					}
				last;
				}
			print "</select>\n";
			print "</span></p>\n";
			close(PRINTERS) || die "\$!=$!, \$?=$?";
			if(scalar @browser_comments > 0)
				{
				my $html = html(join("\n", @browser_comments));
				$html =~ s/\n/<br>\n/g;
				return $html;
				}
			return "";
			},
		'onnext' => sub {
				my $browser_printer = cgi_data_peek("browser_printer", undef);
				if(! defined($browser_printer))
					{ return _("You must choose a printer!") }
				#if($browser_printer !~ /^([^,]+),"([^"]+)"(?:,"([^"]+)")(?:,([^,]+))(?:,([^,]+))(?:,([^,]+))$/)
				if($browser_printer !~ /^([^,]+),"([^"]+)"$/)
					{ return "internal error: browser_printer=$browser_printer" }
				$data{interface} = $1;
				$data{address} = $2;
				$data{options} = defined $3 ? $3 : "";
				$data{jobbreak} = defined $4 ? $4 : "default";
				$data{feedback} = defined $5 ? $5 : "default";
				$data{codes} = defined $6 ? $6 : "default";
				return undef;
				}
		},
		
		#===========================================
		# Select a PPD file
		#===========================================
		#'picture' => "wiz-ppd.jpg",
		{
		'label' => 'ppd',
		'title' => N_("PPR: Add a Printer: Choose a PPD File"),
		'dopage' => sub {
				require "ppd_select.pl";

				# PPD file, if any, selected on a previous pass through this form.
				my $ppd = cgi_data_move('ppd', undef);

				print "<p>",
						H_("Please select a appropriate PostScript Printer\n"
						. "Description (PPD) file for this printer."),
						"</p>\n";

				print "<table class=\"ppd\"><tr><td>\n";

				{
				my $ppd_probe = cgi_data_move("ppd_probe", "");
				if($ppd_probe eq "probe")
					{
					$data{ppd_probe_list} = ppd_probe($data{interface}, $data{address}, $data{options});
					}
				elsif($ppd_probe eq "clear")
					{
					delete $data{ppd_probe_list};
					}
				}

				my $ppd_probe_list = cgi_data_peek('ppd_probe_list', "");
				print '<p><label>';
				if($ppd_probe_list ne "")
					{
					print H_("Suitable PPD Files:");
					}
				else
					{
					print H_("All Available PPD Files:");
					}
				print "<br>\n";
				print '<select tabindex=1 name="ppd" size="15" style="max-width: 300px" onchange="forms[0].submit();">', "\n";
				my $lastgroup = "";
				foreach my $item (ppd_list(cgi_data_peek('ppd_probe_list', "")))
					{
					my($item_manufacturer, $item_modelname) = @{$item};
					if($item_manufacturer ne $lastgroup)
						{
						print "</optgroup>\n" if($lastgroup ne "");
						print "<optgroup label=", html_value($item_manufacturer), ">\n";
						$lastgroup = $item_manufacturer;
						}
					print "<option value=", html_value($item_modelname);
					print " selected" if($item_modelname eq $ppd);
					print ">", html($item_modelname), "\n";
					}
				print "</optgroup>\n" if($lastgroup ne "");
				print "</select>\n";
				print "</p>\n";

				print "</td><td style=\"padding-top: 1cm;\">\n";

				# Print a small table with a summary of what the PPD files says.
				if($ppd ne "")
					{
					ppd_summary($ppd);
					}

				print "</td></tr></table>\n";

				if(cgi_data_peek("ppd_probe_list","") eq "")
					{
					isubmit("ppd_probe", "probe", N_("Auto Detect"), _("Automatically detect printer type and propose suitable PPD files."));
					}
				else
					{
					isubmit("ppd_probe", "clear", N_("Show All PPD Files"));
					}
				},
		'onnext' => sub {
				if(! defined($data{ppd}))
					{ return _("You must choose a PPD file first!") }
				return undef;
				}
		},

		#===========================================
		# Name printer
		#===========================================
		{
		'title' => N_("PPR: Add a Printer: Name the Printer"),
		'picture' => "wiz-newprn.jpg",
		'dopage' => sub {
				print "<p>", H_("The printer must have a name.  The name may be up\n"
						. "to 16 characters long.  Control codes, tildes, and spaces\n"
						. "are not allowed.  Also, the first character may not be\n"
						. "a period or a hyphen."), "</p>\n";

				my $name = cgi_data_move("name", "");
				if($name eq "")
					{
					$name = suggest_queue_name(cgi_data_peek("interface", ""), cgi_data_peek("address", ""));
					}

				print "<p><span class=\"label\">", H_("Printer Name:"), "</span><br>\n";
				print "<input tabindex=1 TYPE=\"text\" SIZE=16 NAME=\"name\" VALUE=", html_value($name), ">\n";
				print "</p>\n";
				},
		'onnext' => sub {
				if($data{name} eq "")
					{ return _("You must enter a name for the printer!") }
				if(-f "$CONFDIR/printers/$data{name}")
					{ return sprintf(_("The printer \"%s\" already exists!"), $data{name}) }
				return undef;
				}
		},

		#===========================================
		# Assign a comment to the printer
		#===========================================
		{
		'title' => _("PPR: Add a Printer: Describe the Printer"),
		'picture' => "wiz-name.jpg",
		'dopage' => sub {
				print "<p>", html(sprintf(_("While short printer names are convenient in certain contexts,\n"
						. "it is sometimes helpful to have a longer, more informative\n"
						. "description.  Please supply a longer description for\n"
						. "\"%s\" below."), $data{name})), "</p>\n";

				my $comment= cgi_data_move("comment", "");
				if($comment eq "")
					{
					$comment = suggest_queue_comment(cgi_data_peek("interface", ""), cgi_data_peek("address", ""));
					}

				print "<p><span class=\"label\">", H_("Description:"), "</span> ";
				print "<input tabindex=1 TYPE=\"text\" SIZE=40 NAME=\"comment\" VALUE=", html_value($comment), ">\n";
				print "</p>\n";
				},
		'onnext' => sub {
				if($data{comment} eq '')
					 { return _("You must supply a description!") }
				return undef;
				}
		},

		#===========================================
		# Additional information
		#===========================================
		{
		'title' => N_("PPR: Add a Printer: Additional Information"),
		'picture' => "wiz-name.jpg",
		'dopage' => sub {
				print "<p>", H_("On this screen you may record additional information\n"
						. "about the printer for future reference."), "</p>\n";

				print "<p>", H_("Where is the printer located?"), "</p>\n";

				print "<p><span class=\"label\">", H_("Location:"), "</span><br>\n";
				print "<input tabindex=1 name=\"location\" size=50 value=", html_value(cgi_data_move('location', '')), ">\n";
				print "</p>\n";

				print "<p>", H_("Which department within your organization uses this printer?"), "</p>\n";

				print "<p><span class=\"label\">", H_("Department:"), "</span><br>\n";
				print "<input tabindex=1 name=\"department\" size=50 value=", html_value(cgi_data_move('department', '')), ">\n";
				print "</p>\n";

				print "<p>", H_("Whom should the print server's system administrator contact to discuss problems with the printer?"), "</p>\n";

				print "<p><span class=\"label\">", H_("On Site Contact:"), "</span><br>\n";
				print "<input tabindex=1 name=\"contact\" size=50 value=", html_value(cgi_data_move('contact', '')), ">\n";
				print "</p>\n";
				},
		'buttons' => [N_("_Cancel"), N_("_Back"), N_("_Finish")]
		},

		#===========================================
		# Save
		#===========================================
		{
		'title' => N_("Save New Printer"),
		'picture' => "wiz-save.jpg",
		'dopage' => sub {
				require 'cgi_run.pl';
				print "<p><span class=\"label\">", H_("Saving new printer:"), "</span></p>\n";
				print "<pre>\n";
				run("id");
				defined($PPAD_PATH) || die;
				my @PPAD = ($PPAD_PATH, "--su", $ENV{REMOTE_USER});
				my $name = cgi_data_peek("name", "?");
				run(@PPAD, 'interface', $name, $data{interface}, $data{address});
				run(@PPAD, 'options', $name, cgi_data_peek("options", ""));
				run(@PPAD, 'jobbreak', $name, $data{jobbreak});
				run(@PPAD, 'feedback', $name, $data{feedback});
				run(@PPAD, 'codes', $name, $data{codes});
				run(@PPAD, 'ppd', $name, $data{ppd});
				run(@PPAD, 'comment', $name, $data{comment});
				if($data{location} ne '')
					{ run(@PPAD, 'location', $name, $data{location}) }
				if($data{department} ne '')
					{ run(@PPAD, 'department', $name, $data{department}) }
				if($data{contact} ne '')
					{ run(@PPAD, 'contact', $name, $data{contact}) }
				run($PPR2SAMBA_PATH, '--nocreate');
				print "</pre>\n";

				# Make the display queues screen reload.  We really should
				# try to make sure there is one first!
				print "<script>window.opener.gentle_reload()</script>\n";
				},
		'buttons' => [N_("_Close")]
		}
];

#===========================================
# Main
#===========================================

&cgi_read_data();

&do_wizard($addprn_wizard_table,
		{
		'auth' => 1,
		'imgdir' => "../images/"
		});

exit 0;

#====================================================================
# On the basis of the interface name and printer address, take a
# wild guess as to what would make a good queue name.  If we can't
# even guess, we just return an empty string.
#====================================================================
sub suggest_queue_name
	{
	my($interface, $address) = @_;
	my $name = "";

	# AppleTalk is pretty easy.  We just remove noise.
	if($interface eq "atalk" && $address =~ /^([^:]+):/)
		{
		$name = $1;
		$name =~ s/_[^_]+_((Direct)|(Print)|(Hold))$//;			# Hack for Canon
		}

	# SocketAPI/AppSocket/JetDirect are embedded, go with the hostname.
	elsif(defined($interface_pages{$interface}) 
			&& $interface_pages{$interface} eq "int_tcpip"
			&& $address =~ /^([^\.:]+)/)
		{
		$name = $1;
		}

	# If this looks like an embedded print server sharename, go with
	# the server name, otherwise, go with the share name.
	elsif($interface eq "smb" && $address =~ /^\\\\([^\\]+)\\([^\\]+)$/)
		{
		my($server, $share) = ($1, $2);
		$share =~ tr/[A-Z]/[a-z]/;
		if($share eq "print" || $share eq "direct" || $share eq "hold" || $share =~ /^lpt/)
			{
			$name = $server;
			}
		else
			{
			$name = $share;
			}
		}

	# Assuming this is a real host and not an embedded print server,
	# the queue name is the most meaningful part.
	elsif($interface eq "lpr" && $address =~ /^([^\@]+)\@/)
		{
		$name = $1;
		}

	# Since most PROS servers generall one port jobs, assume the
	# hostname is the most meaningful thing.
	elsif($interface eq "pros" && $address =~ /\@([^\@]+)$/)
		{
		$name = $1;
		}

	# Convert to lower case.  Sure PPR on Unix can distinguish queue names
	# that differ only in case, but not all operating systems can, it
	# confuses users, and it is harder to type.	 So there!
	$name =~ tr/[A-Z]/[a-z]/;

	# Remove characters that Unix shell users won't like.
	$name =~ s/[^[a-z0-9_-]//g;

	# Truncate to 16 characters (which is PPR's current limit).
	$name =~ s/^(.{1,16}).*$/$1/;

	return $name;
	}

#====================================================================
# On the basis of the interface name, printer address, and PPD file
# name try to suggest a good printer comment.
#====================================================================
sub suggest_queue_comment
	{
	my($interface, $address, $ppdfile) = @_;
	my $name = "";

	# If it is an AppleTalk address and the name part contains
	# at least one space,
	if($interface eq "atalk" && $address =~ /^([^:]+\s[^:]+):/)
		{
		$name = $1;
		}

	return $name;
	}

# end of file
