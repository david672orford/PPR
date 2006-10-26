#
# mouse:~ppr/src/www/cgi_menu.pl
# Copyright 1995--2006, Trinity College Computing Center.
# Written by David Chappell.
#
# This file is part of PPR.  You can redistribute it and modify it under the
# terms of the revised BSD licence (without the advertising clause) as
# described in the accompanying file LICENSE.txt.
#
# Last modified 25 October 2006.
#

#============================================================================
# Menu bar widgets
#============================================================================

sub menu_tools
	{
	menu_start("m_tools", _("Tools"));
		menu_link(_("Cookie Login"), "../html/login_cookie.html",
			_("Use this if your browser doesn't support Digest authentication."));
		menu_link(_("Spooler Logs"), "../html/show_logs.html",
			_("Open a window with links to the spooler log files."));
		menu_link(_("Disk Space"), "df_html.cgi",
			_("Display information about free disk space on the print server."));
		menu_link(_("Tests"), "../html/test.html",
			_("Open a window with links to test scripts."));
	menu_end();
	}

sub menu_window
	{
	my($qtype, $qname) = @_;
	menu_start("m_window", _("Window"));
		menu_link(_("Queues"), "show_queues.cgi",
			_("Open a window with an icon for each queue."));
		menu_link(_("All Jobs"), "show_jobs.cgi",
			_("Open a window which lists all jobs."));
		if(defined $qtype && $qtype ne "all" && defined $qname)	
			{
			if($qtype eq "printer")
				{
				menu_link(sprintf(_("Control %s"), $qname), "prn_control.cgi?" . form_urlencoded("name", $qname),
					_("Monitor, start, stop, and mount forms on the printer."));
				menu_link(sprintf(_("Properties of %s"), $qname), "prn_properties.cgi?" . form_urlencoded("name", $qname),
					_("Examine and change the configuration of the printer."));
				}
			elsif($qtype eq "group")
				{
				menu_link(sprintf(_("Control %s members"), $qname), "grp_control.cgi?" . form_urlencoded("name", $qname),
					_("Monitor, start, stop, and mount forms on the member printers."));
				menu_link(sprintf(_("Properties of %s"), $qname), "grp_properties.cgi?" . form_urlencoded("name", $qname),
					_("Examine and change the configuration of the group."));
				}
			}
	menu_end();
	}

sub menu_help
	{
	$ENV{SCRIPT_NAME} =~ m#([^/]+)\.cgi$# || die;
	my $basename = $1;
	menu_start("m_help", _("Help"));
		menu_link((_("For This Window")), "../help/$basename." . help_language() . ".html",
			_("Display the help document for this program in a web browser window"));
		menu_link(_("All PPR Documents"), "../docs/",
			_("Open the PPR documentation index in a web browser window"));
		menu_link(_("About PPR"), "about.cgi",
			_("Display PPR version information"));
	menu_end();
	}

sub menu_start
    {
	my($id, $name) = @_;
	print "\t<a href=\"#\" onclick=\"return popup2(this,'$id')\">", html($name), '</a>';
	print '<div class="popup"', " id=\"$id\"", ' onmouseover="offmenu(event)"><table cellspacing="0">', "\n";
	}

sub menu_end
	{
	print "\t</table></div>\n";
	}

# This creates a series of menu table rows each of which contains one of a set
# of radio buttons.
sub menu_radio_set
	{
	my($name, $values, $current_value, $extra) = @_;
	foreach	my $value (@{$values})
		{
		print "\t\t", '<tr><td><input type="radio" name=', html_value($name), ' id=', html_value($name . "_" . $value->[0]), ' value=', html_value($value->[0]);
		print ' checked' if($value->[0] eq $current_value);
		print " ", $extra if(defined $extra);
		print '></td><td><label for=', html_value($name . "_" . $value->[0]), '>', html($value->[1]), "</label></td></tr>\n";
		}
	}

# This creates a menu table row which contains a hyperlink.
sub menu_link
	{
	my($label, $url, $tooltip) = @_;
	print "\t\t<tr><td></td><td>";
	print "\t\t<a href=\"$url\" target=\"_blank\" onclick=\"return wopen(event,this.href)\" title=", html_value($tooltip), ">", html($label), "</a>\n";
	print "\t\t</td></tr>\n";
	}

# This creates a menu submit button.  Don't forget that $value must be untranslated.
sub menu_submit
	{
	print "\t\t<tr><td></td><td>";
	isubmit(@_);
	print "\t\t</td></tr>\n";
	}

1;
