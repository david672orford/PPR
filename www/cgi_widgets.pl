#
# mouse:~ppr/src/www/cgi_widgets.pl
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
# Last modified 23 December 2003.
#

# This creates a checkbox followed by a label.  If the box is checked,
# then $name will be set to $value and POSTed.  This widget should be
# used for multiple checkboxes which share the same name.
sub labeled_checkbox
	{
	my($name, $label, $value, $checked) = @_;
	print "<label><input type=\"checkbox\" name=\"$name\" value=", html_value($value);
	print " checked" if($checked);
	print "> ", html($label), "</label>\n";
	}

# This creates a checkbox followed by a label.  If the box is checked,
# then $name will be posted with a value which evaluates to true
# in Perl.
sub labeled_boolean
	{
	my($name, $label, $value) = @_;
	print "<label><input type=\"checkbox\" name=\"$name\" value=\"1\"";
	print " checked" if($value);
	print "> ", html($label), "</label>\n";
	}

# This creates a radio button followed by a label.
sub labeled_radio
	{
	my($name, $label, $value, $current_value) = @_;
	print "<label><input type=\"radio\" name=\"$name\" value=", html_value($value);
	print " checked" if($value eq $current_value);
	print "> ", html($label), "</label>\n";
	}

# This creates an labeled entry widget.  If the entry widget is 50 or more
# characters wide, then it is displayed on the line after the label.
sub labeled_entry
	{
	my($name, $label, $value, $size, $tooltip, $extra) = @_;
	print "<label";
	print " title=", html_value($tooltip) if(defined $tooltip);
	print ">";
	if(defined $label)
		{
		print html($label);
		print $size >= 50 ? "<br>\n " : " ";
		}
	print "<input name=\"$name\" size=$size value=", html_value($value);
	print " $extra" if(defined $extra);
	print ">";
	print "</label>\n";
	}

# This creates a labeled 'blank' with a filled-in value.  The value
# is ordinary HTML text, so it isn't editable.
sub labeled_blank
	{
	my($label, $value, $size) = @_;
	print '<span class="label">', html($label), "</span> ";
	print '<span class="value">', html($value);
	$size -= length($value);
	while($size-- > 0)
		{
		print "&nbsp;";
		}
	print "</span>\n";
	}

# This creates a link which looks like a button.
sub link_button
	{
	my($label, $url, $tooltip) = @_;
	print "<a class=\"buttons\" href=\"$url\" target=\"_blank\" onclick=\"return wopen(null,this.href)\" title=", html_value($tooltip), ">", html($label), "</a>\n";
	}

# This creates a button that pops up a help page.  The name of the help page is
# based on the script name.	 The argument is used to make an HTML fragment
# identifier.
sub help_button
	{
	my $helpdir = shift;
	my $topic = shift;

	$helpdir = "" if(!defined($helpdir));

	$ENV{SCRIPT_NAME} =~ m#([^/]+)\.cgi$# || die;
	my $basename = $1;

	my $lang = defined $ENV{LANG} ? $ENV{LANG} : "en";

	my $append = defined($helpdir) ? "" : "_help";

	my $helpfile = "$helpdir$basename$append.$lang.html";

	my $fragment = defined($topic) ? "#$topic" : "";

	print '<a class="buttons" href="', $helpfile, $fragment, '" target="_blank"',
		" onclick=\"window.open(this.href,'_blank','menubar,resizable,scrollbars,toolbar');return false;\">",
		H_("Help"),
		"</a>\n";
	}

# This creates a labeled select box with the default value indicated.
sub labeled_select
	{
	my $name = shift;
	my $label = shift;
	my $default = shift;
	my $selected = shift;
	my @options = @_;

	print "<label>";
	print html($label) if(defined $label);
	print "<select name=\"$name\">\n";
	foreach my $option (@options)
		{
		print " <option value=", html_value($option);
		print " selected" if($option eq $selected);
		print ">", H_($option);
		if($option eq "default")
			{ print " (", H_($default), ")" }
		print "\n";
		}
	print " </select>\n";
	print "</label>\n";
	}

# This differs from labeled_select() in that there is no provision for marking
# the default value, there can be a tooltip, and extra parameters (such as
# onchange=) can be inserted into the <select> tag.
sub labeled_select2
    {
	my($name, $label, $values, $current_value, $tooltip, $extra) = @_;
	print '<label title=', html_value($tooltip), ">\n";
	print html_nb($label), '&nbsp;';
	print '<select name=', html_value($name);
	print " ", $extra if(defined $extra);
	print ">\n";

	foreach	my $value (@{$values})
		{
		print '<option';
		print ' selected' if($value->[0] eq $current_value);
		print ' value=', html_value($value->[0]), '>', html($value->[1]), "\n";
		}

	print "</select>\n";
	print "</label>\n";
	}

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
		menu_link(_("For This Window"), "../help/$basename." . (defined $ENV{LANG} ? $ENV{LANG} : "en") . ".html",
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
	print '<div class="popup" name="menubar"', " id=\"$id\"", ' onmouseover="offmenu(event)"><table cellspacing="0">', "\n";
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
		print "\t\t", '<tr><td><label><input type="radio" name=', html_value($name), ' value=', html_value($value->[0]);
		print ' checked' if($value->[0] eq $current_value);
		print " ", $extra if(defined $extra);
		print '>', html($value->[1]), "</label></td></tr>\n";
		}
	}

# This creates a menu table row which contains a hyperlink.
sub menu_link
	{
	my($label, $url, $tooltip) = @_;
	print "\t\t<tr><td>";
	print "\t\t<a href=\"$url\" target=\"_blank\" onclick=\"return wopen(event,this.href)\" title=", html_value($tooltip), ">", html($label), "</a>\n";
	print "\t\t</td></tr>\n";
	}

# This creates a menu submit button.  Don't forget that $value must be untranslated.
sub menu_submit
	{
	print "\t\t<tr><td>";
	isubmit(@_);
	print "\t\t</td></tr>\n";
	}

1;
