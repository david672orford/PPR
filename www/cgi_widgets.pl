#
# mouse:~ppr/src/www/cgi_widgets.pl
# Copyright 1995--2002, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appears in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is"
# without express or implied warranty.
#
# Last modified 24 April 2002.
#

sub labeled_checkbox
    {
    my($name, $label, $value, $checked) = @_;
    print "<input type=\"checkbox\" name=\"$name\" value=\"$value\"";
    print " checked" if($checked);
    print "> <span class=\"label\">", html($label), "</span>\n";
    }

sub labeled_select
    {
    my $name = shift;
    my $label = shift;
    my $default = shift;
    my $selected = shift;
    my @options = @_;

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
    }

sub labeled_entry
    {
    my($name, $label, $value, $size) = @_;
    if(defined $label)
	{
	print '<span class="label">', html($label), "</span> ";
	print "<br>\n " if($size >= 50);
	}
    print "<input name=\"$name\" size=$size value=", html_value($value), ">\n";
    }

sub labeled_blank
    {
    my($label, $value, $size) = @_;
    print '<span class="label">', html($label), "</span> ";
    print "<span class=\"value\">", html($value);
    $size -= length($value);
    while($size-- > 0)
	{
	print "&nbsp;";
	}
    print "</span>\n";
    }

sub labeled_boolean
    {
    my($name, $label, $value) = @_;
    print "<input type=\"checkbox\" name=\"$name\" value=\"1\"";
    print " checked" if($value);
    print "> <span class=\"label\">", html($label), "</span>";
    print "\n";
    }

1;
