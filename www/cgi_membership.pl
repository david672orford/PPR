#
# mouse:~ppr/src/www/cgi_membership.pl
# Copyright 1995--2000, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appears in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is"
# without express or implied warranty.
#
# Last modified 3 February 2000.
#

#
# This function produces the HTML for two select boxes with buttons which
# can be used to move items back and forth.
#
# There can be more than one of these per wizard or per tabbed dialog, but
# then cannot be on the same screen because of the way CGI variables are used.
#
sub cgi_membership
    {
    my($members_title, $members_var, $members_size,
	$nonmembers_title, $nonmembers_var, $nonmembers_size) = @_;

    # convert space separated members list to an array.
    my @members = split(/ /, $data{$members_var});
    my @nonmembers = split(/ /, $data{$nonmembers_var});

    # Act on move buttons.
    if(defined($data{move}))
        {
        my $move = &cgi_data_move('move', '');
        my $ni = &cgi_data_move('nonmembers_select', '-1');
        my $mi = &cgi_data_move('members_select', '-1');

        if($move eq '<-' && $ni != -1)
            {
            push(@members, $nonmembers[$ni]);
            splice(@nonmembers, $ni, 1);
            }

        elsif($move eq '->' && $mi != -1)
            {
            push(@nonmembers, $members[$mi]);
            splice(@members, $mi, 1);
            }

        elsif($move eq 'v' && $mi != -1)
            {
            push(@members, $members[$mi]);
            splice(@members, $mi, 1);
            }

        elsif($move eq '^' && $mi != -1)
            {
            unshift(@members, $members[$mi]);
            splice(@members, $mi + 1, 1);
            }
        }

    # Start the nested table.
    print "<table><tr>\n";

    # Create the up and down move buttons to the left
    # of the members select box.
    print '<td align="right">', "\n";
    vspace($members_size);
    print '<input tabindex=10 accesskey="^" type="submit" name="move" value="^">', "\n";
    print "<br>\n";
    print '<input tabindex=11 accesskey="v" type="submit" name="move" value="v">', "\n";
    print "</td>\n";

    # Create the members select box.
    print "<td>\n";
    print "<label><span class=\"label\">", html($members_title), "</span><br>\n";
    print "<select tabindex=1 accesskey=\"m\" name=\"members_select\" size=$members_size>\n";
    {
    my $i = 0;
    foreach $_ (@members)
        {
        print "<option value=\"$i\">", html($_), "\n";
        $i++;
        }
    }
    print "<option value=\"-1\">________________\n";
    print "</select></label>\n";
    print "</td>\n";

    # Create the left and right buttons in the middle.
    print "<td>\n";
    vspace($nonmembers_size);
    print '<input tabindex=2 accesskey="&gt;" type=submit name="move" value="-&gt;">', "\n";
    print "<br>\n";
    print '<input tabindex=4 accesskey="&lt;" type="submit" name="move" value="&lt;-">', "\n";
    print "</td>\n";

    # Create the non-members select box.
    print "<td>\n";
    print "<label><span class=\"label\">", html($nonmembers_title), "</span><br>\n";
    print "<select tabindex=3 accesskey=\"o\" name=\"nonmembers_select\" size=$nonmembers_size>\n";
    {
    my $i = 0;
    foreach $_ (@nonmembers)
        {
        print "<option value=\"$i\">", html($_), "\n";
        $i++;
        }
    }
    print "<option value=\"-1\">________________\n";
    print "</select></label>\n";
    print "</td>\n";

    # That is the end of the nested table.
    print "</table>\n";

    # Pack the two lists back into simple variables.
    $data{$nonmembers_var} = join(' ', @nonmembers);
    $data{$members_var} = join(' ', @members);
    }

sub vspace
    {
    my $x = shift;
    $x = ($x / 2) - 2;
    while($x-- > 0)		# Values such as 7.5 will never be 0.00!
    	{
    	print "<br>\n";
    	}
    }

1;

