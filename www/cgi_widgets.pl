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
# Last modified 8 August 2003.
#

sub labeled_checkbox
	{
	my($name, $label, $value, $checked) = @_;
	print '<span class="widget">';
	print "<input type=\"checkbox\" name=\"$name\" value=\"$value\"";
	print " checked" if($checked);
	print "> <span class=\"label\">", html($label), "</span>";
	print "</span>\n";
	}

sub labeled_select
	{
	my $name = shift;
	my $label = shift;
	my $default = shift;
	my $selected = shift;
	my @options = @_;

	print '<span class="widget">';
	print '<span class="label">', html($label), "</span>", " " if(defined $label);
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
	print "</span>\n";
	}

sub labeled_entry
	{
	my($name, $label, $value, $size) = @_;
	print '<span class="widget">';
	if(defined $label)
		{
		print '<span class="label">', html($label), "</span> ";
		print "<br>\n " if($size >= 50);
		}
	print "<input name=\"$name\" size=$size value=", html_value($value), ">";
	print "</span>\n";
	}

sub labeled_blank
	{
	my($label, $value, $size) = @_;
	print '<span class="widget">';
	print '<span class="label">', html($label), "</span> ";
	print "<span class=\"value\">", html($value);
	$size -= length($value);
	while($size-- > 0)
		{
		print "&nbsp;";
		}
	print "</span>";
	print "</span>\n";
	}

sub labeled_boolean
	{
	my($name, $label, $value) = @_;
	print '<span class="widget">';
	print "<input type=\"checkbox\" name=\"$name\" value=\"1\"";
	print " checked" if($value);
	print "> <span class=\"label\">", html($label), "</span>";
	print "</span>\n";
	}

#
# Here is a button that pops up a help page.  The name of the help page is
# based on the script name.	 The argument is used to make an HTML fragment
# identifier.
#
sub help_button
{
my $helpdir = shift;
my $topic = shift;

$ENV{SCRIPT_NAME} =~ m#([^/]+)\.cgi$# || die;
my $basename = $1;

my $lang = defined $ENV{LANG} ? $ENV{LANG} : "en";

my $append = defined($helpdir) ? "" : "_help";

my $helpfile = "$helpdir$basename$append.$lang.html";

my $fragment = defined($topic) ? "#$topic" : "";

print <<"HelpLink";
<span class="buttons">
<a href="$helpfile$fragment" target="_blank"
		onclick="window.open('$helpfile$fragment','_blank','width=600,height=400,resizable,scrollbars');return false;">
		${\H_("Help")}</a>
</span>
HelpLink
}

1;
