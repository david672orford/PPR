#! /usr/bin/perl -wT
#
# mouse:~ppr/src/misc/prn_addwiz.cgi.perl
# Copyright 1995--2003, Trinity College Computing Center.
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
# Last modified 5 April 2003.
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
require 'cgi_run.pl';

defined($HOMEDIR) || die;
defined($INTDIR) || die;
defined($CONFDIR) || die;
defined($PPR2SAMBA_PATH) || die;

# Programs we need:
$GETZONES = "$HOMEDIR/lib/getzones";
$NBP_LOOKUP = "$HOMEDIR/lib/nbp_lookup";

#===========================================
# This is a table of interface program
# descriptions.
#===========================================
%interface_descriptions = (
		'dummy' => "--hide",
		'simple' => N_("Server Generic Port"),
		'serial' => N_("Server Serial Port"),
		'parallel' => N_("Server Parallel Port"),
		'usblp' => N_("Server USB Port"),
		'atalk' => N_("AppleTalk PAP"),
		'tcpip' => N_("SocketAPI (JetDirect)"),
		'lpr' => N_("RFC 1179 (lpr/lpd protocol)"),
		'smb' => N_("LAN Manager/MS-Windows"),
		'ppromatic' => "--hide",
		'gssimple' => "--hide",
		'gsatalk' => "--hide",
		'gstcpip' => "--hide",
		'gsserial' => "--hide",
		'gsparallel' => "--hide",
		'gslpr' => "--hide",
		'gssmb' => "--hide"
		);

#====================================================================
# On the basis of the interface name and printer address, take a
# wild guess as to what would make a good queue name.  If we can't
# even guess, we just return an empty string.
#====================================================================
sub suggest_queue_name
	{
	my($interface, $address) = @_;
	my $name = "";

	# Don't treat Ghostscript versions differently.
	$interface =~ s/^gs//;

	if($interface eq "atalk" && $address =~ /^([^:]+):/)
		{
		$name = $1;
		$name =~ s/_[^_]+_((Direct)|(Print)|(Hold))$//;			# Hack for Canon
		}
	elsif($interface eq "tcpip" && $address =~ /^([^\.:]+)/)
		{
		$name = $1;
		}
	elsif($interface eq "smb" && $address =~ /^\\\\[^\\]+\\([^\\]+)$/)
		{
		$name = $1;
		}
	elsif($interface eq "lpr" && $address =~ /^([^\@]+)\@/)
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

	# Don't treat Ghostscript versions differently.
	$interface =~ s/^gs//;

	# If it is an AppleTalk address and the name part contains
	# at least one space,
	if($interface eq "atalk" && $address =~ /^([^:]+\s[^:]+):/)
		{
		$name = $1;
		}

	return $name;
	}

#===========================================
# This is the table for this wizard:
#===========================================
$addprn_wizard_table = [
		#===========================================
		# Welcome
		#===========================================
		{
		'title' => N_("Add a Printer"),
		'picture' => "wiz-newprn.jpg",
		'dopage' => sub {
				print "<p><span class=\"label\">", H_("PPR Printer Queue Creation"), "</span></p>\n";
				print "<p>", H_("This program will guide you through the process of setting up a printer in PPR."), "</p>\n";
				},
		'buttons' => [N_("_Cancel"), N_("_Next")]
		},

		#===========================================
		# Choose an interface
		#===========================================
		{
		'label' => 'choose_int',
		'title' => N_("Select an Interface Program"),
		'picture' => "wiz-interface.jpg",
		'dopage' => sub {
				# Get a sorted list of the available interfaces.
				opendir(I, $INTDIR) || die "opendir() failed on \"$INTDIR\", $!";
				my @interfaces = sort(grep(!/^\./, readdir(I)));
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
				while(defined($interface = shift @interfaces))
					{
					if($thiscol++ > 9)
						{
						print "</td><td>\n";
						$thiscol = 0;
						}
					my $desc = $interface_descriptions{$interface};
					next if(defined($desc) && $desc eq "--hide");
					print "<input tabindex=1 TYPE=\"radio\" NAME=\"interface\" VALUE=", html_value($interface);
					print " checked" if($interface eq $checked_interface);
					print "> $interface";
					print " - <font size=-3>", H_($desc), "</font>" if(defined($desc));
					print "<BR>\n";
					}
				print "</td></tr></table>\n";
				},
		'onnext' => sub {
				if(! defined($data{interface}))
					{ return _("You must select an interface!") }
				return undef;
				},
		'getnext' => sub {
				my $interface = $data{interface};

				# Where do we go next?
				if($interface eq 'atalk')
					{
					$data{'int_atalk_type'} = 'LaserWriter';
					return 'int_atalk';
					}
				if($interface eq 'tcpip' || $interface eq 'gstcpip')
					{
					return 'int_tcpip';
					}
				if($interface eq 'lpr')
					{
					return 'int_lpr';
					}
				if($interface eq 'gsatalk')
					{
					$data{'int_atalk_type'} = 'DeskWriter';
					return 'int_atalk';
					}
				return 'int_generic';
				}
		},

		#===========================================
		# Interface setup for atalk, page 1
		#===========================================
		{
		'label' => 'int_atalk',
		'title' => N_("AppleTalk Interface: Choose a Zone"),
		'picture' => "wiz-address.jpg",
		'dopage' => sub {
				my $zone = cgi_data_move('int_atalk_zone', '');

				# Use the getzones program to fetch the zone list and sort it.
				opencmd(GZ, $GETZONES) || die "Unable to get zone list: $!\n";
				my @zlist = sort(<GZ>);
				close(GZ) || die $!;

				print "<p>", H_("Please choose the zone which contains the printer you want to use."), "</p>\n";

				print "<p><span class=\"label\">", H_("AppleTalk Zones:"), "</span><br>\n";

				print "<select tabindex=1 name=\"int_atalk_zone\" size=12>\n";
				foreach $i (@zlist)
					{
					chomp $i;
					print "<option value=", html_value($i);
					print " selected" if($i eq $zone);
					print ">", html($i), "\n";
					}
				print "</select>\n";

				print "</p>\n";

				print "<p>", H_("When you press the [Next] button, the selected zone will\n"
						. "be searched.  This will take about 10 seconds."), "</p>\n";
				},
		'onnext' => sub {
				if($data{'int_atalk_zone'} eq '')
					{ return _("You must choose a zone before you may proceed!") }
				return undef;
				}
		},

		#===========================================
		# Interface setup for atalk, page 2
		#===========================================
		{
		'label' => 'int_atalk',
		'title' => N_("AppleTalk Interface: Choose a Printer"),
		'picture' => "wiz-address.jpg",
		'dopage' => sub {
				my $address = cgi_data_move('int_atalk_address', '');
				my $zone = $data{'int_atalk_zone'};
				my $type = $data{'int_atalk_type'};

				print "<p>", H_("Please choose the printer you want to use from the list below."), "</p>\n";

				print "<p><span class=\"label\">", H_("Available Printers:"), "</span><br>\n";

				opencmd(GPR, $NBP_LOOKUP, "=:$type\@$zone") || die "Unable to get printer list: $!\n";
				my @plist = ();
				while(<GPR>)
					{
					chomp;
					s/^\d+:\d+:\d+ \d+ //;
					push(@plist, $_);
					}
				close(GPR) || die;
				@plist = sort(@plist);

				print "<select tabindex=1 name=\"int_atalk_address\" size=12>\n";
				foreach my $i (@plist)
					{
					print "<option value=", html_value($i);
					print " selected" if($i eq $address);
					print ">", html($i), "\n";
					}
				print "</select>\n";

				print "</p>\n";
				},
		'onnext' => sub {
				my $address;

				if(($address = $data{int_atalk_address}) eq '')
					{ return _("You must select a printer before you can proceed!") }

				# Strip quotes off of printer and zone.
				$address =~ s/^"([^"]*)"$/$1/;

				$data{address} = $address;
				$data{options} = '';
				$data{jobbreak} = 'default';
				$data{feedback} = 'default';

				return undef;
				},
		'getnext' => sub { return 'ppd' }
		},

		#===========================================
		# Interface setup for TCP/IP JetDirect
		# style printing
		#===========================================
		{
		'label' => 'int_tcpip',
		'title' => N_("Raw TCP/IP Interface: Choose an Address"),
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
								. "With some other equipment, the port number the printer\n"
								. "accepts jobs on can be changed but is 9100 when they\n"
								. "come from the factory."), "</p>\n";

				print "<p>", H_("TCP Port Number:"), " ";
				print "<input tabindex=1 name=\"int_tcpip_port\" size=8 value=", html_value(cgi_data_move('int_tcpip_port', '9100')), ">\n";
				print "</p>\n";

				# If this is for the Ghostscript version of the tcpip interface,
				# then it is bidirectional because we have a two way communictions
				# chanel with the PostScript interpreter (Ghostscript).
				if($data{interface} =~ /^gs/)
					{
					$data{int_tcpip_feedback} = "default";
					}

				# Otherwise, this is a question for the user.
				else
					{
					print "<p>", H_("Internal JetDirect cards support bidirectional communication.\n"
						. "External print servers will only support bidirectional\n"
						. "communication if they communicate with the printer\n"
						. "through a serial port or through a bidirectional parallel port."), "</p>\n";

					print "<p>", H_("Bidirectional:"), " ";
					print "<select tabindex=1 name=\"int_tcpip_feedback\">\n";
					my $feedback = cgi_data_move('int_tcpip_feedback', 'Yes');
					print "<option";
					print " selected" if($feedback eq 'Yes');
					print ">Yes\n";
					print "<option";
					print " selected" if($feedback eq 'No');
					print ">No\n";
					print "</select>\n</p>\n";
					}
				},
		'onnext' => sub {
				if($data{'int_tcpip_name'} eq '')
					{ return _("You must fill in the printer's DNS name or IP address!") }
				if($data{'int_tcpip_port'} eq '')
					{ return _("You must fill in the TCP port number!") }
				$data{address} = $data{int_tcpip_name} . ':' . $data{int_tcpip_port};
				$data{options} = '';
				$data{jobbreak} = 'default';
				$data{feedback} = $data{'int_tcpip_feedback'};

				return undef;
				},
		'getnext' => sub { return 'ppd' }
		},

		#===========================================
		# Interface setup for RFC 1179 servers.
		#===========================================
		{
		'label' => 'int_lpr',
		'title' => N_("LPR Interface: Choose an Address"),
		'picture' => "wiz-address.jpg",
		'dopage' => sub {
				print "<p>", H_("It is necessary to know the IP address of the server,\n"
						. "or preferably a DNS name which represents that address.\n"
						. "An example of a DNS name is \"myhost.myorg.org\".  An example\n"
						. "of an IP address is \"157.252.200.52\"."), "</p>\n";

				print "<p>", H_("DNS Name or IP Address:"), " ";
				print "<input tabindex=1 name=\"int_lpr_host\" size=32 value=", html_value(cgi_data_move('int_lpr_host', '')), ">\n";
				print "</p>\n";

				print "<p>", H_("The remote queue name may be different from the local queue name.\n"
						. "If so, change it below."), "</p>\n";

				print "<p>", H_("Remote Queue Name:"), " ";
				print "<input tabindex=1 name=\"int_lpr_printer\" size=16 value=", html_value(cgi_data_move('int_lpr_printer', $data{'name'})), ">\n";
				print "</p>\n";
				},
		'onnext' => sub {
				if($data{int_lpr_host} eq '')
					{ return _("You must fill in the remote host's DNS name or IP address!") }
				if($data{int_lpr_printer} eq '')
					{ return _("You must fill in the remote queue name!") }
				$data{address} = $data{int_lpr_printer} . '@' . $data{int_lpr_host};
				$data{options} = '';
				$data{jobbreak} = 'default';
				$data{feedback} = 'default';
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
		'title' => N_("Interface Setup"),
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
				$data{feedback} = $data{int_generic_feedback};
				$data{jobbreak} = $data{int_generic_jobbreak};
				return undef;
				},
		'getnext' => sub {
				return 'ppd';
				}
		},

		#===========================================
		# Select a PPD file
		#===========================================
		#'picture' => "wiz-ppd.jpg",
		{
		'label' => 'ppd',
		'title' => N_("Choose a PPD File"),
		'dopage' => sub {
				require "ppd_select.pl";

				# PPD file, if any, selected on a previous pass through this form.
				my $ppd = cgi_data_move('ppd', undef);
				my $ppd_description = "";

				print "<p><span class=\"label\">", H_("Please select a appropriate PostScript Printer\n"
						. "Description (PPD) file for this printer:"), "</span><br><br>\n";
				print "<table class=\"ppd\"><tr><td>\n";

				print '<select tabindex=1 name="ppd" size="15" style="max-width: 300px" onchange="forms[0].submit();">', "\n";
				#print "<option>\n";
				my $lastgroup = "";
				foreach my $item (ppd_list())
					{
					my($item_file, $item_manufacturer, $item_description) = @{$item};
					if($item_manufacturer ne $lastgroup)
						{
						print "</optgroup>\n" if($lastgroup ne "");
						print "<optgroup label=", html_value($item_manufacturer), ">\n";
						$lastgroup = $item_manufacturer;
						}
					print "<option value=", html_value($item_file);
					print " selected" if($item_file eq $ppd);
					print ">", html($item_description), "\n";
					if($item_file eq $ppd)
						{
						$ppd_description = $item_description;
						}
					}
				print "</select>\n";
				print "</p>\n";

				print "</td><td>\n";

				# Print a small table with a summary of what the PPD files says.
				if($ppd ne "")
					{
					ppd_summary($ppd, $ppd_description);
					}

				print "</td></tr></table>\n";
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
		'title' => N_("Add a Printer"),
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
		'title' => _("Describe the Printer"),
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
		'title' => N_("Additional Information"),
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
				print "<p><span class=\"label\">", H_("Saving new printer:"), "</span></p>\n";
				print "<pre>\n";
				run("id");
				defined($PPAD_PATH) || die;
				my @PPAD = ($PPAD_PATH, "--su", $ENV{REMOTE_USER});
				my $name = cgi_data_peek("name", "?");
				run(@PPAD, 'interface', $name, $data{interface}, $data{address});
				run(@PPAD, 'options', $name, cgi_data_peek("options", ""));
				run(@PPAD, 'ppd', $name, $data{ppd});
				run(@PPAD, 'feedback', $name, $data{feedback});
				run(@PPAD, 'jobbreak', $name, $data{jobbreak});
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

# end of file
