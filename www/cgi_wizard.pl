#
# mouse:~ppr/src/www/cgi_wizard.pl
# Copyright 1995--2002, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is"
# without express or implied warranty.
#
# Last modified 16 August 2002.
#

use 5.004;
require "cgi_data.pl";
require "cgi_intl.pl";

#=======================================
# Print out the appropriate page from
# the wizard page table.
#=======================================
sub do_wizard
{
my $wizard_table = shift;
my $options = shift;

# Assign default values to unset options.
$options->{auth} = 0 if(!defined($options->{auth}));
$options->{debug} = 0 if(!defined($options->{debug}));
$options->{prefix} = "../" if(!defined($options->{prefix}));
$options->{jsdir} = "$options->{prefix}js/" if(!defined($options->{jsdir}));
$options->{cssdir} = "$options->{prefix}style/" if(!defined($options->{cssdir}));
$options->{wiz_imgdir} = "$options->{prefix}images/" if(!defined($options->{wiz_imgdir}));
$options->{imgdir} = "" if(!defined($options->{imgdir}));
$options->{imgheight} = 128 if(!defined($options->{imgheight}));
$options->{imgwidth} = 128 if(!defined($options->{imgwidth}));

# Turn on table borders if we are running in debug mode.
my $border = $options->{debug} ? 1 : 0;

# Initialize the internationalization libraries and determine the
# content language and charset.
my ($charset, $content_language) = cgi_intl_init();

# If there is no record of which page we are on,
# this must be a new wizard on the first page.
my $new_wizard = 0;
if(! defined($data{wiz_page}) || ! defined($data{wiz_stack}))
    {
    $data{wiz_page} = 0;
    $data{wiz_stack} = "";
    $new_wizard = 1;
    }

# Which page are we on and what button was pressed (if any).
my $page = &cgi_data_move("wiz_page", "");
my $action = &cgi_data_move("wiz_action", "");

# Break the stack variable out into an array.  The stack keeps a trail back
# thru the pages we have traversed.  When the users presses our Back button,
# this stack is popped.
my @stack = split(/ /, $data{wiz_stack});

# Debugging code
#print STDERR "Page: $page\n";
#print STDERR "Action: $action\n";
#print STDERR "Stack: ", join(' ', @stack), "\n";

# If this is this is the first appearance of this wizard or this is the
# Finish page, demand authentication.  We ask for authentication on the
# first page even though it is not strictly necessary so as not to raise
# false hopes.
if($options->{auth} && ($new_wizard || $action eq "Finish"))
    {
    if($ENV{REMOTE_USER} eq "")
    	{
	require "cgi_auth.pl";
	demand_authentication();
	return;
    	}
    }

# If this is defined, it will be printed in red with
# an exclamation point next to it.
my $error = undef;

# If the user has pressed a button:
if($action ne "")
    {
    my $wizpage = $wizard_table->[$page];

    # If the user has pressed the "Next" button
    # or the "Finish" button which is equivelent,
    if($action eq 'Next' || $action eq 'Finish')
	{
	# If there is no "onnext" procedure for this page
	# or the onnext procedure when called returns
	# undef instead of an error message, then move
	# on to the next page.
	my $onnext = $wizpage->{onnext};
	if(!defined($onnext) || !defined($error = &$onnext))
	    {
	    push(@stack, $page);

	    # If there is a "getnext" procedure, then call
	    # it and if it returns a value other than undef,
	    # go to that page.
	    my $getnext;
	    my $next_goto;
	    if(defined($getnext = $wizpage->{getnext}) && defined($next_goto = &$getnext))
		{
		print STDERR "Goto: $next_goto\n" if($options->{debug} > 1);
		while(1)
		    {
		    $page++;
		    die if($page > $#$wizard_table);
		    last if(defined $wizard_table->[$page]->{label} && $wizard_table->[$page]->{label} eq $next_goto);
		    }
		}
	    # If there is no "getnext" procedure, then simply
	    # move to the next page in the list.
	    else
		{
		$page++;
		die if($page > $#$wizard_table);
		}
	    }
	}

    # If the user pressed the "Back" button,
    elsif($action eq "Back")
	{
	# If we aren't on the first page,
	if($page > 0)
	    {
	    $page = pop(@stack);
	    }
	}

    # If the user pressed the "Cancel" button
    # and Javascript isn't enabled.
    elsif($action eq "Cancel" || $action eq "Close")
	{
	require 'cgi_back.pl';
	cgi_back_doit();
	return;
	}

    # Invalid form post:
    else
	{
	die "Unknown button \"$action\"";
	}
    } # if button pressed

# Form a pointer to the current page.
$wizpage = $wizard_table->[$page];

# Find the title for this page and the
# image which decorates its left hand
# side.
my $title = H_($wizpage->{title});
my $picture = $wizpage->{picture};
#$picture = "missing.jpg" if(!defined($picture));
my $picture_alt = $wizpage->{picture_alt};
if(defined $picture && !defined($picture_alt))
    {
    if($picture =~ /([^\/]+)\.[^\/]+$/)
	{ $picture_alt = "[$1]" }
    else
	{ $picture_alt = $picture }
    }

# Get options for table alignment.
my $align = $wizpage->{align};
$align = "left" if(!defined($align));
my $valign = $wizpage->{valign};
$valign = "center" if(!defined($valign));

# Start the page and the HTML form it contains.
print <<"EndOfText1";
Content-Type: text/html;charset=$charset
Content-Language: $content_language
Vary: accept-language

<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd">
<html>
<head>
<title>$title</title>
<link rel="stylesheet" href="$options->{cssdir}cgi_wizard.css" type="text/css">
</head>
<body>
<form action="$ENV{SCRIPT_NAME}" method="POST">
EndOfText1

# Start the table which holds the decorative picture on the
# left and the functional part of the form on the right.
print <<"EndOfText2a";
<TABLE BORDER=$border HEIGHT=85% WIDTH=100% cellpadding=15 cols=4>
<tr height=400>
<td width=1 align="left" valign="top">
EndOfText2a

if(defined $picture)
{
print <<"EndOfText2b";
  <img src="$options->{imgdir}$picture" height=$options->{imgheight} width=$options->{imgwidth} alt=${\&html_value($picture_alt)}>
EndOfText2b
}

print <<"EndOfText2c";
  </td>
<td align=$align valign=$valign colspan=3>
EndOfText2c

print "\n  <!-- start of dopage() output -->\n\n" if($options->{debug} > 0);

# If there is a "dopage" procedure, then call it to emit
# the text which distinguishes this page from others.
my $dopage = $wizpage->{dopage};
if(defined($dopage))
    {
    eval { &$dopage };
    if($@)			# if error,
    	{			# print message from die
	print "<p>$@</p>\n";
    	}
    }

print "\n  <!-- end of dopage() output -->\n\n" if($options->{debug} > 0);

print "</td>\n</tr>\n";

# This debug information helps to make the table rendering clearer.
if($options->{debug} > 1)
    {
    print "<tr>";
    my $x;
    for($x=1; $x <= 5; $x++)
    	{
    	print "<td>$x</td>";
    	}
    print "</tr>\n";
    }

# Close the table which holds the picture and
# the main page text and start another to hold
# the error text (if any) and the submit buttons
# labeled "Back" and "Next".
print <<"EndOfText2";
</TABLE>
<TABLE BORDER=$border HEIGHT="10%" WIDTH="100%" COLS=4 cellpadding=5>
<tr>
<td colspan=3 height=80>
EndOfText2

# If there is error text, put it here in red.
if(defined($error))
    {
print <<"EndOfText4";
<img alt="!" src="$options->{wiz_imgdir}exclaim.png" height=64 width=32>
<span class="alert">${\&html($error)}</span>
EndOfText4
    }

# Now the submit buttons, then we close the table.  Note that we
# call the subroutine isubmit() to create the buttons.  It internationalizes
# the buttons if called for.
print <<"EndOfText6";
</td>
<td nowrap align="right" valign="bottom">
EndOfText6

{
my $buttons = $wizpage->{buttons};
if(!defined($buttons)) { $buttons = [ N_("_Cancel"), N_("_Back"), N_("_Next") ] }
my $b;
my $tabindex = 1000;
foreach $b (@$buttons)
    {
    my $other = "class=\"buttons\" tabindex=$tabindex";
    if($b eq "_Cancel" || $b eq "_Close")
	{ $other .= " onclick=\"self.close()\"" }
    (my $b_stript = $b ) =~ s/_//;
    isubmit("wiz_action", $b_stript, $b, $other);
    $tabindex--;
    }
}

print <<"EndOfText8";
</td>
</tr>
</table>
EndOfText8

# Update the page and stack variable:
$data{wiz_page} = $page;
$data{wiz_stack} = join(' ', @stack);

# Emmit the data gathered on other pages as hidden
# form fields.
&cgi_write_data();

print "</form>\n";

# Print data at bottom of page for debugging.
&cgi_debug_data() if($options->{debug});

# Snap the window size to fit snugly around the document.
print <<"Tail05";
<script>
if(document.width)
	{
	window.resizeTo(document.width + 20, document.height + 20);
	}
</script>
Tail05

# And this is the last of the HTML document.
print <<"Tail10";
</body>
</html>
Tail10
} # end of do_wizard()

1;

