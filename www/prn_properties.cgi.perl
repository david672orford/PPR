#! /usr/bin/perl -wT
#
# mouse:~ppr/src/www/prn_properties.cgi.perl
# Copyright 1995--2002, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is" without
# express or implied warranty.
#
# Last modified 26 April 2002.
#

use lib "?";
require 'paths.ph';
require 'cgi_data.pl';
require 'cgi_tabbed.pl';
require 'cgi_intl.pl';
require 'cgi_widgets.pl';

defined($INTDIR) || die;
defined($PPDDIR) || die;
defined($PPR2SAMBA_PATH) || die;

my $tabbed_table = [
	#====================================================
	# Pane for the print queue comment
	#====================================================
	{
	'tabname' => N_("Comment"),
	'help' => "comment",
	'dopage' => sub {

		print "<p>";
		labeled_blank(_("Printer name:"), cgi_data_peek("name", "???"), 50);

		print "<p>";
		labeled_entry("comment", _("Comment:"), cgi_data_move("comment", ""), 50);

		print "<p>";
		labeled_entry("location", _("Location:"), cgi_data_move("location", ""), 50);

		print "<p>";
		labeled_entry("department", _("Department:"), cgi_data_move("department", ""), 50);

		print "<p>";
		labeled_entry("contact", _("Contact:"), cgi_data_move("contact", ""), 50);
		}
	},

	#====================================================
	# Pane for the interface settings
	#====================================================
	{
	'tabname' => N_("Interface"),
	'dopage' => sub {
		# Read a list of available interfaces into @interface_list.
		opendir(I, $INTDIR) || die "opendir() failed on \"$INTDIR\", $!";
		my @interface_list = ();
		while(defined(my $interface = readdir(I)))
		    {
		    next if($interface =~ /^\./);
		    push(@interface_list, $interface);
		    }
		closedir(I) || die;

		print "<p>";
		labeled_select("interface", _("Printer interface program:"), "", cgi_data_move("interface", ""), @interface_list);

		print "<p>";
		labeled_entry("address", _("Printer address:"), cgi_data_move("address", ""), 50);

		print "<p>";
		labeled_entry("options", _("Interface options:"), cgi_data_move("options", ""), 50);

		{
		print "<p><span class=\"label\">", H_("Does feedback (2-way communication) work?"), "</span><br>\n";
		my $selected_feedback = cgi_data_move('feedback', 'default');
		foreach my $feedback (N_("default"), N_("yes"), N_("no"))
		    {
		    print '<input type="radio" name="feedback" value=', html_value($feedback);
		    if($feedback eq $selected_feedback)
		    	{ print " checked" }
		    print "> ", H_($feedback);
		    if($feedback eq 'default')
		    	{ print " (", H_($data{feedback_default}), ")" }
		    print "\n";
		    }
		}

		print "<p>";
		labeled_select("jobbreak", _("Jobbreak method:"),
				$data{jobbreak_default}, cgi_data_move("jobbreak", "default"),
				N_("default"),
				N_("control-d"),
				N_("pjl"),
				N_("signal"),
				N_("signal/pjl"),
				N_("newinterface"),
				N_("none"),
				N_("save/restore"));

		print "<p>";
		labeled_select("codes", _("Character code compatibility:"),
				$data{codes_default}, (split(/ /, cgi_data_move("codes", "default")))[0],
				N_("default"),
				N_("Clean7Bit"),
				N_("Clean8Bit"),
				N_("Binary"),
				N_("TBCP"),
				N_("UNKNOWN"));

		},
	'onleave' => sub {
		if($data{interface} eq "")
		    { return _("No interface is selected!") }
		if($data{address} eq "")
		    { return _("The Printer Address is blank!") }
		return undef;
		}
	},

	#====================================================
	# Pane to select the PPD file
	#====================================================
	{
	'tabname' => N_("PPD"),
	'help' => "ppd",
	'dopage' => sub {
		my $checked_ppd = cgi_data_move('ppd', undef);

		print "<span class=\"label\">", H_("PPD file:"), "</span>\n";

		print "<table class=\"ppd\"><tr><td>\n";
		
		# Read the PPD File list (and unfortunately "." and "..") into
		# the array @ppd_list.
		opendir(P, $PPDDIR) || die "opendir() failed on \"$PPDDIR\", $!";
		my @ppd_list = sort(readdir(P));
		closedir(P) || die;

		# Print the HTML for a select box.
		print "<select name=\"ppd\" size=12>\n";
		if(defined($checked_ppd))
		    {
		    print "<option value=", html_value($checked_ppd), " selected>", html($checked_ppd), "\n";
		    }
		foreach my $ppd (@ppd_list)
		    {
		    next if($ppd =~ /^\./);
		    print "<option value=", html_value($ppd), ">", html($ppd), "\n";
		    }
		print "</select>\n";

		print "</td><td>\n";

		my @trivia = ppd_trivia($checked_ppd);
		print "<table class=\"lines\" cellspacing=0>\n";
		print "<tr><th colspan=2>", html(sprintf(_("Features of %s"), $checked_ppd)), "</th></tr>\n";
		print "<tr><th>", H_("PPD File Version"), "</th><td>", html($trivia[0]), "</td></tr>\n";
		print "<tr><th>", H_("LanguageLevel"), "</th><td>", html($trivia[1]), "</td></tr>\n";
		print "<tr><th>", H_("PostScript Version"), "</th><td>", html($trivia[2]), "</td></tr>\n";
		print "<tr><th>", H_("Number of Fonts"), "</th><td>", html($trivia[3]), "</td></tr>\n";
		print "<tr><th>", H_("TrueType Rasterizer"), "</th><td>", html($trivia[4]), "</td></tr>\n";
		print "</table>\n";

		print "</td></tr></table>\n";
		},
	'onleave' => sub {
		if($data{ppd} eq '')
		    { return _("No PPD file is selected!") }
		return undef;
		}
	},

	#====================================================
	# Pane to select the RIP
	#====================================================
	{
	'tabname' => N_("RIP"),
	'dopage' => sub {

		# Split the RIP settings into three values.
		($data{rip_name}, $data{rip_output_language}, $data{rip_options}) = split(/\t/, cgi_data_move("rip", "\t\t"));

		# Handle the complex business with the sections and
		# their radio buttons.
		fix_rip_which() if(!defined $data{rip_which});
		my $rip_which = $data{rip_which_previous} = cgi_data_move("rip_which", undef);
		
		# Upper section: PPD RIP info.
		{
		print "<div class=\"section\">\n";
		print "<span class=\"section_label\"><input type=\"radio\" name=\"rip_which\" value=\"ppd\"";
		print " checked" if($rip_which eq "ppd");
		print "> From PPD File</span>\n";

		my $ppdfile = cgi_data_peek("ppd", undef);
		my @rip_list;
		if(! defined $ppdfile)
		    {
		    print "<p>", H_("No PPD file selected in [PPD] pane."), "</p>\n";
		    }
		elsif(scalar(@rip_list = ppd_rip($ppdfile)) == 0)
		    {
		    print "<p>", H_("The PPD file does not call for a raster image processor."), "</p>\n";
		    }
		else
		    {
		    print "<p>";
		    labeled_blank(_("Raster Image Processor:"), $rip_list[0], 8);
		    print "<p>";
		    labeled_blank(_("Output Language:"), $rip_list[1], 8);
		    print "<p>";
		    labeled_blank(_("Options:"), $rip_list[2], 40);
		    }

		print "</div>\n";
		}

		# Lower section: Custom RIP Info
		{
		print "<div class=\"section\">\n";
		print "<span class=\"section_label\"><input type=\"radio\" name=\"rip_which\" value=\"custom\"";
		print " checked" if($rip_which eq "custom");
		print "> Custom</span>\n";

		print "<p>";
		labeled_select("rip_name", _("Raster Image Processor:"),
			"", cgi_data_move("rip_name", undef),
			"", "gs", "ppr-gs");

		print "<p>";
		labeled_select("rip_output_language", _("Output Language:"),
			"", cgi_data_move("rip_output_language", undef),
			"", "pcl", "other");

		print "<p>";
		labeled_entry("rip_options", _("Options:"), cgi_data_move("rip_options", undef), 40);

		print "</div>\n";
		}
		},
	'onleave' => sub {
		my $error = fix_rip_which();

		$data{rip} = join("\t",
		    	cgi_data_move("rip_name", ""),
		    	cgi_data_move("rip_output_language", ""),
		    	cgi_data_move("rip_options", ""));
		$data{rip} =~ s/\t+$//;

		return $error;
		}
	},

	#====================================================
	# This is for printer hardware options enumerated
	# in the PPD file.
	#====================================================
	{
	'tabname' => N_("Features"),
	'dopage' => sub {
		print "<span class=\"label\">", H_("Optional printer features:"), "</span><br>\n";

		# Get the list of features from the PPD file.
		my @features = ppd_features(cgi_data_peek("ppd", "?"));

		# Create a hash of the current settings.
		my %current;
		{
		my @list = split(/ /, cgi_data_move("ppdopts", ""));
		my $name;
		my $value;
		while(defined($name = shift @list))
		    {
		    $value = shift @list;
		    $current{"$name $value"} = 1;
		    #print "<pre>\$name=\"$name\", \$value=\"$value\"</pre>\n";
		    }
		}

		# Print a select control for each feature.
		foreach my $feature (@features)
		    {
		    #print "<pre>", join(' ', @$feature), "</pre>\n";
		    my $name = shift @$feature;
		    $name =~ m#^([^/]+)/?(.*)$# || die;
		    my($name_mr, $name_tr) = ($1, $2);
		    print "<select name=\"ppdopts\">\n";
		    print "<option value=\"\">", html($name_tr), "\n";
		    foreach my $setting (@$feature)
		    	{
			$setting =~ m#^([^/]+)/?(.*)$# || die;
			my($setting_mr, $setting_tr) = ($1, $2);
			print "<option value=", html_value("$name_mr $setting_mr");
			print " selected" if(defined $current{"$name_mr $setting_mr"});
			print ">", html("$name_tr $setting_tr"), "\n";
		    	}
		    print "</select><br>\n";
		    }
		}
	},

	#====================================================
	# Bin management
	#====================================================
	{
	'tabname' => N_("Bins"),
	'dopage' => sub {
		print "<span class=\"label\">", H_("Bins for forms management purposes:"), "</span><br>\n";

		my %current_bins = ();
		foreach my $bin (split(/ /, cgi_data_move("bins", "")))
		    {
		    $current_bins{$bin} = 1;
		    }

		foreach my $ppd_bin (ppd_bins(cgi_data_peek("ppd", "?")))
		    {
		    my ($name, $translation) = @$ppd_bin;
		    my $checked = defined($current_bins{$name});
		    labeled_checkbox("bins", sprintf($translation eq "" ? "%s" : _("%s -- %s"), $name, $translation), $name, $checked);
		    print "<br>\n";
		    delete $current_bins{$name} if($checked);
		    }

		foreach my $bin (sort keys %current_bins)
		    {
		    labeled_checkbox("bins", ($bin . _(" (not listed in PPD file)")), $bin, 1);
		    print "<br>\n";
		    }
		}
	},

	#====================================================
	# Operator alerting method
	#====================================================
	{
	'tabname' => N_("Alerts"),
	'dopage' => sub {
		print "<p>";
		labeled_select("alerts_method", _("Alert method:"),
			"", cgi_data_move("alerts_method", ""),
			N_("mail"));

		print "<p>";
		labeled_select("alerts_frequency_sign", _("Alert frequency:"),
			"", cgi_data_move("alerts_frequency_sign", ""),
			N_("never"),
			N_("after"),
			N_("every"));
		labeled_entry("alerts_frequency_abs", undef, cgi_data_move("alerts_frequency_abs", ""), 5);
		print H_("errors"), "\n";

		print "<p>";
		labeled_entry("alerts_address", _("Alert address:"), cgi_data_move("alerts_address", ""), 50);
		}
	},

	#====================================================
	# Switchset
	#====================================================
	{
	'tabname' => N_("Switchset"),
	'dopage' => sub {
		print "<p><span class=\"label\">", H_("Switchset:"), "</span><br>\n";
		print "<textarea name=switchset cols=50 rows=10>\n";
		print html(&cgi_data_move('switchset', ''));
		print "</textarea>\n";
		print "</p>\n";
		}
	},

	#====================================================
	# Samba
	#====================================================
	{
	'tabname' => N_("Samba"),
	'dopage' => sub {
		print "<div class=\"section\">\n";
		print "<span class=\"section_label\">";
		labeled_checkbox("addon ppr2samba", _("Share with Samba"), 1, cgi_data_move("addon ppr2samba", 1));
		print "</span>\n";

		print "<p>";
		labeled_select("addon ppr2samba-prototype", _("Prototype Share:"),
			"", cgi_data_move("addon ppr2samba-prototype", ""), 
			"", "pprproto", "pprproto_pprpopup", "pprproto_pprpopup2");
		print "</p>\n";

		print "<p>";
		labeled_entry("addon ppr2samba-drivername", _("Override Win95 driver name:"), cgi_data_move("addon ppr2samba-drivername", ""), 20);
		print "</p>\n";

		print "<p>";
		labeled_select("addon ppr2samba-vserver", _("Assign to virtual server (Samba setup required):"),
			"", cgi_data_move("addon ppr2samba-vserver", ""),
			"", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10");
		print "</p>\n";

		print "</div>\n";

		print "<p>", _("Note that if the Samba configuration file smb.conf has not been edited as described in\n"
				. "the ppr2samba(8) manpage, the settings on this screen will have no effect."), "</p>\n";

		},
	'onleave' => sub {
		# This gives it a value of 0 if it wasn't checked and blank if it was.
		$data{"addon ppr2samba"} = (cgi_data_move("addon ppr2samba", 0) ? "" : "0");
		return undef;
		}
	},

	#====================================================
	# PAP
	#====================================================
	{
	'tabname' => N_("PAP"),
	'dopage' => sub {
		print "<div class=\"section\">\n";
		print "<span class=\"section_label\">";
		labeled_checkbox("addon ppr2samba", _("Share with AppleTalk PAP"), 0, cgi_data_move("addon pprpapd", 0));
		print "</span>\n";

		print "<p>";
		labeled_entry("addon pprpapd-papname", _("Share As:"), cgi_data_move("addon pprpapd-papname", ""), 32);
		print "</p>\n";
		
		print "</div>\n";
		}
	},

	#====================================================
	# Limits
	#====================================================
	{
	'tabname' => N_("Limits"),
	'dopage' => sub {
		print "<div class=\"section\">\n";
		print "<span class=\"section_label\">", H_("Limits Enforced Before Printing"), "</span>\n";

		print '<p><span class="label">', H_("Limit Kilobytes"), "</span><br>\n";
		labeled_entry("limitkilobytes_lower", _("Lower limit:"), cgi_data_move("limitkilobytes_lower", 0), 8);
		labeled_entry("limitkilobytes_upper", _("Upper limit:"), cgi_data_move("limitkilobytes_upper", 0), 8);
		print "</p>\n";

		print '<p><span class="label">', H_("Limit Pages"), "</span><br>\n";
		labeled_entry("limitpages_lower", _("Lower limit:"), cgi_data_move("limitpages_lower", 0), 8);
		labeled_entry("limitpages_upper", _("Upper limit:"), cgi_data_move("limitpages_upper", 0), 8);
		print "</p>\n";

		print "<p>";
		labeled_select("grayok", _("Grayscale documents OK:"), "", cgi_data_move("grayok", "yes"), N_("yes"), N_("no"));
		print "</p>\n";

		print "</div>\n";

		print "<div class=\"section\">\n";
		print "<span class=\"section_label\">", H_("Limits Enforced During Printing"), "</span>\n";

		print "<p>";
		labeled_entry("pagetimelimit", H_("Per-page time limit (in seconds):"), cgi_data_move("pagetimelimit", 0), 4);
		print "</p>\n";

		print "</div>\n";
		}
	},

	#====================================================
	# Userparams
	#====================================================
	{
	'tabname' => N_("Userparams"),
	'dopage' => sub {
		print '<span class="label">', H_("PostScript Interpreter Userparams Settings:"), "</span><br>\n";

		my %userparams;
		$userparams{waittimeout} = ['timeout', "WaitTimeout", undef];
		$userparams{jobtimeout} = ['timeout', "JobTimeout", undef];
		$userparams{manualfeedtimeout} = ['timeout', "ManualfeedTimeout", undef];
		$userparams{doprinterrors} = ['bool', "DoPrintErrors", undef];

		# Scan the existing list and save the capitalization and current
		# value in the master table.
		foreach my $pair (split(/\s+/, cgi_data_move("userparams", "")))
		    {
		    if($pair =~ /^([^=]+)=(.+)$/)
		    	{
			my($name, $value) = ($1, $2);
			my $name_lowered = $name;
			$name_lowered =~ tr/A-Z/a-z/;

			if(!defined $userparams{$name_lowered})
			    {
			    if($value eq "false" || $value eq "true")
			    	{
			   	$userparams{$name_lowered}->[0] = "bool";
			    	}
			    if($name =~ /Timeout$/i)
			    	{
				$userparams{$name_lowered}->[0] = "timeout";
			    	}
			    }

                        $userparams{$name_lowered}->[1] = $name;
                        $userparams{$name_lowered}->[2] = $value;
		    	}
		    }

		# Generate a control for each value.
		foreach my $control (keys %userparams)
		    {
		    my($type, $name, $value) = @{$userparams{$control}};
		    if($type eq "bool")
		    	{
			print "<select name=\"userparams\">";
			print "<option value=\"\"";
				print " selected" if(!defined $value);
				print ">", html($name), "=default\n";
			print "<option value=", html_value("$name=false");
				print " selected" if($value eq "false");
				print ">", html($name), "=false\n";
			print "<option value=", html_value("$name=true");
				print " selected" if($value eq "true");
				print ">", html($name), "=true\n";
			print "</select><br>\n";
		    	}
		    elsif($type eq "timeout")
		    	{
			print "<select name=\"userparams\">";
			print "<option value=\"\"";
				print " selected" if(!defined $value);
				print ">", html($name), "=default\n";
			foreach my $i (qw(0 30 45 60 90 120 180 240 360 480 720 1800))
			    {
			    if($value < $i)
			    	{
				print "<option value=", html_value("$name=$value"), " selected>", html("$name=$value"), "\n";
				$value = 1000000;
			    	}
			    print "<option value=", html_value("$name=$i");
			    if($i == $value)
			    	{
			    	print " selected";
				$value = 1000000;
			    	}
			    print ">", html("$name=$i"), "\n";
			    }
			print "</select><br>\n";
		    	}
		    else
		    	{
			print "<input name=\"userparams\" value=", html_value("$name=$value"), " size=25><br>\n";
		    	}
		    }

		# Leave one extra space
		print "<input name=\"userparams\" value=\"\" size=25><br>\n";
		},
	'onleave' => sub {
		# Remove valueless options from the userparams.
		$data{userparams} = join(" ", grep(/^[^=]+=[^=]+/, split(/\s+/, $data{userparams})));
		return undef;
		}
	},

	#====================================================
	# Other
	#  Flag Pages
	#  Charge
	#  Output Order
	#====================================================
	{
	'tabname' => N_("Other"),
	'dopage' => sub {
		{
		my @flags_list = (N_("never"), N_("no"), N_("yes"), N_("always"));
		print "<p>\n";
		labeled_select("flags_banner", _("Print banner page"), "", cgi_data_move("flags_banner", ""), @flags_list);
		print "<br>\n";
		labeled_select("flags_trailer", _("Print trailer page"), "", cgi_data_move("flags_trailer", ""), @flags_list);
		print "</p>\n";
		}

		print "<p>";
		print "<span class=\"label\">", H_("Monetary Charge for Printing on this Printer"), "<br>\n";
		labeled_entry("charge_duplex", _("Per duplex sheet:"), cgi_data_move("charge_duplex", ""), 6);
		labeled_entry("charge_simplex", _("Per simplex sheet:"), cgi_data_move("charge_simplex", ""), 6);
		print "</p>\n";

		print "<p>";
		labeled_select("outputorder", _("Output order:"),
			"", cgi_data_move("outputorder", ""),
			N_("Normal"),
			N_("Reverse"),
			N_("PPD"));
		print "</p>\n";

		print "<p>";
		labeled_entry("passthru", _("Passthru printer languages:"), cgi_data_move("passthru", ""), 25);
		print "</p>\n";
		},
	'onleave' => sub {
		if(cgi_data_peek('charge_duplex', '') !~ /^(\d*\.\d\d)?$/)
		    { return _("The charge per duplex sheet is not valid!") }
		if(cgi_data_peek('charge_simplex', '') !~ /^(\d*\.\d\d)?$/)
		    { return _("The charge per simplex sheet is not valid!") }
		if((cgi_data_peek('charge_duplex', '') eq '') != (cgi_data_peek('charge_simplex', '') eq ''))
		    { return _("Both the duplex charge and the simplex charge must be set or neither must be set!") }
		return undef;
		}
	}
];

#=========================================================
# These functions retrieve information from the PPD file.
#=========================================================

#
# This function will return a list of the optional
# features that a certain printer has.
#
sub ppd_features
    {
    require "readppd.pl";
    my $filename = shift;

    my $line;
    my $open_ui = undef;
    my @answer = ();
    my $subanswer;

    ppd_open($filename);

    while(defined($line = ppd_readline()))
	{
	if($line =~ /^\*OpenUI/)
	    {
	    die "Unclosed UI section \"$open_ui\"" if(defined $open_ui);
            if($line =~ /^\*OpenUI\s+(\*Option[^:]*)/ || $line =~ /^\*OpenUI\s+(\*InstalledMemory[^:]*)/)
                {
                $open_ui = $1;
		#print "Start of $open_ui ($open_ui_mr)\n";
                $open_ui =~ m#^([^/]*)# || die;
                $open_ui_mr = $1;
		$subanswer = [$open_ui];
                next;
                }
	    }
	if(defined $open_ui && ($line =~ /^(\*Option\S*)\s*([^:]*)/ || $line =~ /^(\*InstalledMemory\S*)\s*([^:]*)/))
	    {
	    my($option, $value) = ($1, $2);
	    die "Found \"$option\" option in \"$open_ui_mr\" section" if($option ne $open_ui_mr);
	    #print "\t$value\n";
	    push(@$subanswer, $value);
	    next;
	    }
	if(defined $open_ui && $line =~ /^\*CloseUI:\s*(\S*)/)
	    {
	    die "Mismatched \"*CloseUI: $1\" doesn't match \"$open_ui\"" if($1 ne $open_ui_mr);
	    $open_ui = undef;
	    push(@answer, $subanswer);
	    next;
	    }
	}

    return @answer;
    }

#
# This function will return a list of the possible bins
# named in the PPD file.
#
sub ppd_bins
    {
    require "readppd.pl";
    my $filename = shift;

    my $line;
    my @answer = ();

    ppd_open($filename);

    while(defined($line = ppd_readline()))
	{
	if($line =~ /^\*InputSlot\s+([^\/:]+)(\/([^:]+))?/)
	    {
	    my ($name, $translation) = ($1, $3);
	    #print "<pre>\$name=\"$name\", \$translation=\"$translation\"</pre>\n";
	    push(@answer, [$name, $translation]);
	    }
	}

    return @answer;
    }

#
# This function will return a three element list which contains the RIP,
# output_language, and options from the PPD file's *pprRIP: line.
#
sub ppd_rip
    {
    require "readppd.pl";
    my $filename = shift;

    my $line;
    my @answer = ();

    ppd_open($filename);

    while(defined($line = ppd_readline()))
	{
	if($line =~ /^\*pprRIP:\s+(\S+)\s+(\S+)\s+([^\n]+)$/)
	    {
	    @answer = ($1, $2, $3);
	    }
	}

    return @answer;
    }

#
# This function gets interesting information from the PPD file.
#
sub ppd_trivia
    {
    require "readppd.pl";
    my $filename = shift;

    my $line;
    my $fileversion = "?";
    my $languagelevel = 1;
    my $psversion = "?";
    my $fonts = 0;
    my $ttrasterizer = "None";

    ppd_open($filename);

    while(defined($line = ppd_readline()))
	{
	if($line =~ /^\*FileVersion:\s*"([^"]+)"/)
	    {
	    $fileversion = $1;
	    next;
	    }
	if($line =~ /^\*LanguageLevel:\s*"([^"]+)"/)
	    {
	    $languagelevel = $1;
	    next;
	    }
	if($line =~ /^\*PSVersion:\s*"([^"]+)"/)
	    {
	    $psversion = $1;
	    next;
	    }
	if($line =~ /^\*Font\s+/)
	    {
	    $fonts++;
	    next;
	    }
	if($line =~ /^\*TTRasterizer:\s*(\S+)/)
	    {
	    $ttrasterizer = $1;
	    next;
	    }
	}

    return ($fileversion, $languagelevel, $psversion, $fonts, $ttrasterizer);
    }

#=====================================================================
# These functions are helpers for the panes.
#=====================================================================
sub fix_rip_which
    {
    my @names = qw(rip_name rip_output_language rip_options);

    my $rip_which = "ppd";

    foreach my $name (@names)
	{
	if(defined $data{$name} && $data{$name} ne "")
	    {
	    print STDERR "+++ custom because \$data{$name} is \"$data{$name}\"\n";
	    $rip_which = "custom";
	    last;
	    }
	}

    if(defined $data{rip_which_previous})
    	{
	if($data{rip_which} eq "ppd" && $data{rip_which_previous} eq "custom")
	    {
	    print STDERR "+++ clearing custom\n";
	    foreach my $name (@names)
	    	{
		$data{$name} = "";
	    	}
	    $rip_which = "ppd";
	    }
	elsif($data{rip_which} eq "custom" && $data{rip_which_previous} eq "ppd")
	    {
	    print STDERR "+++ switching to custom\n";
	    $rip_which = "custom";
	    }
    	}

    $data{rip_which} = $rip_which;

    if($rip_which eq "custom")
	{
	foreach my $name (@names)
	    {
	    if(cgi_data_peek($name, "") eq "")
		{
		print STDERR "+++ missing $name\n";
		return _("The custom RIP information is incomplete.");
		}
	    }
	}

    return undef;
    }

#============================================
# This function is called from do_tabbed().
# It uses the "ppad show" command to
# get the current printer configuration.
#============================================

sub load
{
require "cgi_run.pl";

my $name = $data{name};
defined($name) || die "No printer name specified!\n";

# Use "ppad -M show" to dump the printer's
# current configuration.
opencmd(PPAD, $PPAD_PATH, '-M', 'show', $name) || die;
while(<PPAD>)
    {
    chomp;
    /^([^\t]+)\t(.*)$/ || die;
    my($key, $value) = ($1, $2);
    $data{$key} = $value;		# copy to modify
    $data{"_$key"} = $value;		# copy to keep
    }
close(PPAD);

# Split the alert information into separate fields.
# Split the signed number alerts_frequency into a count (absolute value)
# and behavior (sign).
my $alerts_frequency;
($alerts_frequency, $data{alerts_method}, $data{alerts_address}) = split(/ /, $data{alerts}, 3);
if($alerts_frequency > 0)
    {
    $data{alerts_frequency_sign} = 'every';
    $data{alerts_frequency_abs} = $alerts_frequency;
    }
elsif($alerts_frequency < 0)
    {
    $data{alerts_frequency_sign} = 'after';
    $data{alerts_frequency_abs} = ($alerts_frequency * -1);
    }
else
    {
    $data{alerts_frequency_sign} = 'never';
    $data{alerts_frequency_abs} = 0;
    }

# Split the charge into the charge per duplex sheet
# and the charge per simplex sheet.  If the second is
# missing, make it a duplicate of the first.
($data{charge_duplex}, $data{charge_simplex}) = split(/ /, cgi_data_move('charge', ' '), 2);

# Split certain interface parameter values into the current
# and default parts.
foreach my $i (qw(jobbreak feedback codes))
    {
    my($current, $default) = split(/ /, $data{$i});
    $data{$i} = $current;
    $data{"_$i"} = $current;
    $data{"${i}_default"} = $default;
    }

# Split the flags setting into banner and trailer.
($data{flags_banner}, $data{flags_trailer}) = split(/ /, cgi_data_move('flags', ' '), 2);

# Split the switchset into lines.
$data{switchset} =~ s/ -/\n-/g;

# Split the limits apart.
($data{limitkilobytes_lower}, $data{limitkilobytes_upper}) = split(/ /, cgi_data_move("limitkilobytes", "0 0"), 2);
($data{limitpages_lower}, $data{limitpages_upper}) = split(/ /, cgi_data_move("limitpages", "0 0"), 2);
}

#============================================
# This function is called from do_tabbed().
# It uses the ppad command to save the
# changes to the printer configuration.
#============================================

sub save
{
require 'cgi_run.pl';

my $name = $data{name};
my $i;

defined($PPAD_PATH) || die;
my @PPAD = ($PPAD_PATH, "--su", $ENV{REMOTE_USER});

print "<p><span class=\"label\">Saving changes:</span></p>\n";
print "<pre>\n";

# Recombine the alert information.
my $alerts_frequency = $data{alerts_frequency_abs};
if($data{alerts_frequency_sign} eq 'after')
    { $alerts_frequency *= -1 }
elsif($data{alerts_frequency_sign} eq 'never')
    { $alerts_frequency = 0 }
$data{alerts} = "$alerts_frequency $data{alerts_method} $data{alerts_address}";

# Recombine certain list things.
$data{flags} = "$data{flags_banner} $data{flags_trailer}";
$data{charge} = "$data{charge_duplex} $data{charge_simplex}";
$data{limitkilobytes} = "$data{limitkilobytes_lower} $data{limitkilobytes_upper}";
$data{limitpages} = "$data{limitpages_lower} $data{limitpages_upper}";

# If the interface or its address have changed,
if($data{interface} ne $data{_interface} || $data{address} ne $data{_address})
    { run(@PPAD, "interface", $name, $data{interface}, $data{address}) }

# If the interface has changed, all of these must be set.  Otherwise,
# only those which have themselves changed need to be set.
foreach my $i (qw(options jobbreak feedback codes))
    {
    if($data{$i} ne $data{"_$i"} || $data{interface} ne $data{_interface})
    	{ run(@PPAD, $i, $name, $data{$i}) }
    }

# Do single value stuff.
foreach my $i (qw(comment location department contact ppd outputorder userparams pagetimelimit grayok))
    {
    if($data{$i} ne $data{"_$i"})
        { run(@PPAD, $i, $name, $data{$i}) }
    }

# Do space separated list value stuff.
foreach my $i (qw (flags charge alerts passthru limitkilobytes limitpages ppdopts))
    {
    if($data{$i} ne $data{"_$i"})
    	{ run(@PPAD, $i, $name, split(/ /, $data{$i}, 100)) }
    }

# Do the tab separated list value stuff.
foreach my $i (qw (rip))
    {
    if($data{$i} ne $data{"_$i"})
    	{ run(@PPAD, $i, $name, split(/\t/, $data{$i}, 100)) }
    }

# Setting bins needs a special command.
if($data{bins} ne $data{"_bins"})
    { run(@PPAD, "bins", "set", $name, split(/ /, $data{bins}, 100)) }

# Switchsets are very hard to do.
{
my $unwrapped_switchset = $data{switchset};
$unwrapped_switchset =~ s/\s*[\r\n]+\s*/ /g;
if($unwrapped_switchset ne $data{"_switchset"})
    {
    run(@PPAD, "switchset", $name, &shell_parse($unwrapped_switchset));
    }
}

# Do all of the "addon" stuff.
my $samba_addon_changed = 0;
foreach my $item (keys %data)
    {
    if($item =~ /^addon (.+)$/)
    	{
	my $key = $1;
	if($data{$item} ne $data{"_$item"})
	    {
	    $samba_addon_changed = 1 if($key =~ /^ppr2samba/);
	    run(@PPAD, "addon", $name, $key, $data{$item});
	    }
    	}
    }

# If anything that Samba cares about has changed, update it.
if($data{comment} ne $data{_comment} || $data{ppd} ne $data{_ppd} || $samba_addon_changed)
    {
    run($PPR2SAMBA_PATH, '--nocreate');
    }

print "</pre>\n";

# Make the display queues screen reload.  We really should
# try to make sure there is one first!
print "<script>window.opener.gentle_reload()</script>\n";
}

#========================================
# main
#========================================

# Should debugging messages be added to the html?
$debug = 0;

# Swap the real and effective user ids.
($<,$>) = ($>,$<);

# Read the CGI POST and query data.
&cgi_read_data();

# Produce the HTML for a tabbed dialog.
&do_tabbed($tabbed_table, sprintf(_("PPR: Printer Properties: %s"), $data{name}), \&load, \&save, 8);

exit 0;
