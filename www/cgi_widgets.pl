#
# mouse:~ppr/src/www/cgi_widgets.pl
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
# Last modified 10 June 2005.
#

=head1 cgi_widgets.pl

This library provides additional HTML widgets.

=over 4

=cut

=item labeled_boolean()

This creates a checkbox followed by a label.  If the box is checked,
then $name will be posted with a value which evaluates to true
in Perl.

Example with default false:

	labeled_boolean(
		"fries",
		_("Will you have fries with that?"),
		cgi_data_move("fries", 0)
		);

Example with default true:

	labeled_boolean(
		"fries",
		_("Will you have fries with that?"),
		cgi_data_move("fries", 1)
		);

=cut
sub labeled_boolean
	{
	my($name, $label, $value) = @_;
	print "<label><input type=\"checkbox\" name=\"$name\" value=\"1\"";
	print " checked" if($value);
	print "> ", html($label), "</label>\n";
	}

=item labeled_checkbox()

This creates a checkbox followed by a label.  If the box is checked,
then $name will be set to $value and POSTed.  This widget should be
used for multiple checkboxes which share the same name.

When multiple values are submitted, the cgi_read_data() function will
join them with spaces in between.  The following sample code splits
them up in order to determine which checkboxes were checked last time.

	my @supplies = split(/ /, cgi_data_move("supplies", ""));
	labeled_checkbox("supplies",
		_("#2 Pencil"),
		"pencil",
		scalar grep($_ eq "pencil", @supplies)	
		);
	labeled_checkbox("supplies",
		_("Pad of Paper"),
		"pad",
		scalar grep($_ eq "pad", @supplies)	
		);

=cut
sub labeled_checkbox
	{
	my($name, $label, $value, $checked) = @_;
	print "<label><input type=\"checkbox\" name=\"$name\" value=", html_value($value);
	print " checked" if($checked);
	print "> ", html($label), "</label>\n";
	}

=item labeled_radio()

This creates a radio button followed by a label.  Use it thus:

	my $surname = cgi_data_move("surname", "");
	labeled_radio("surname", _("Surname is Smith"), "smith", $surname);
	labeled_radio("surname", _("Surname is Jones"), "jones", $surname);

=cut
sub labeled_radio
	{
	my($name, $label, $value, $current_value) = @_;
	print "<label><input type=\"radio\" name=\"$name\" value=", html_value($value);
	print " checked" if($value eq $current_value);
	print "> ", html($label), "</label>\n";
	}

=item labeled_entry()

This creates an labeled entry widget.  If the entry widget is 50 or more
characters wide, then it is displayed on the line after the label.  Use 
it like this:

	labeled_entry(
		"phone_number",
		_("Phone Number:"),
		cgi_data_move("phone_number", ""),
		16
		);

If you want to add a tooltip:

	labeled_entry(
		"phone_number",
		_("Phone Number:"),
		cgi_data_move("phone_number", ""),
		16,
		_("Enter your phone number here.")
		);

You can add a final parameter if you want to add arbitrary attributes.

=cut
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

=item labeled_blank()

This creates a labeled 'blank' with a filled-in value.  The value
is ordinary HTML text, so it isn't editable.

	labeled_blank(
		_("Your appointment date:"),
		cgi_data_peek("appt_date", ""),
		16
		);

=cut
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

=item link_button()

This creates a link which looks like a button.  When clicked on it opens
the page in a new window.  This is good for help buttons.

	link_button(
		_("Help"),
		"help/myhelp.html",
		_("Click here to open a help window.")
		);

=cut
sub link_button
	{
	my($label, $url, $tooltip) = @_;
	print "<a class=\"buttons\" href=\"$url\" target=\"_blank\" onclick=\"return wopen(null,this.href)\" title=", html_value($tooltip), ">", html($label), "</a>\n";
	}

=item help_language()

Prune a language name down to size for a help file.  We do this by removing the
dialect and encoding information leaving only the two-letter language code.

=cut
sub help_language 
	{
	my $lang = defined $ENV{LANG} ? $ENV{LANG} : "en";
	if($lang =~ /^([a-z][a-z])[\._-]/)
		{
		$lang = $1;
		}
	return $lang;
	}

=item help_button()

This creates a button that pops up a help page.  The name of the help page is
based on the script name.  The first argument specifies the directory where 
help files are found.  The second argument (if present) is used to make an
HTML fragment identifier.

	help_button("../help/");
	help_button("../help/", "page2");

=cut
sub help_button
	{
	my $helpdir = shift;
	my $topic = shift;

	$helpdir = "" if(!defined($helpdir));

	$ENV{SCRIPT_NAME} =~ m#([^/]+)\.cgi$# || die;
	my $basename = $1;

	my $lang = help_language();

	my $append = defined($helpdir) ? "" : "_help";

	my $helpfile = "$helpdir$basename$append.$lang.html";

	my $fragment = defined($topic) ? "#$topic" : "";

	my $legend = _("Help");
	$legend =~ s/_//;

	print '<a class="buttons" href="', $helpfile, $fragment, '" target="_blank"',
		" onclick=\"window.open(this.href,'_blank','menubar,resizable,scrollbars,toolbar');return false;\">",
		html($legend),
		"</a>\n";
	}

=item labeled_select()

This creates a labeled select box with the default value indicated.

	labeled_select(
		"telephone_color",
		_("Desired Telephone Color:"),
		"black",
		cgi_data_move("telephone_color", "black"),
		qw(black red blue green yellow white)
		);

=cut
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

=item labeled_select2()

This differs from labeled_select() in that there is no provision for marking
the default value (in its label), there can be a tooltip, and extra parameters 
(such as onchange=) can be inserted into the <select> tag.

	labeled_select2(
		"telephone_color",
		_("Desired Telephone Color:"),
		\qw(black red blue green yellow white),
		cgi_data_move("telephone_color", "black"),
		_("Select the desired color for your new telephone."),
		"onchange='return color_change()'"
		);

=cut
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

=back

=cut

1;
