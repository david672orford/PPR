#
# mouse:~ppr/src/www/cgi_tabbed.pl
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
# Last modified 23 March 2004.
#

use 5.004;
require 'cgi_data.pl';
require 'cgi_intl.pl';

#
# These are the values that are used if individual pages don't override.
#
my $DEFAULT_cellpadding = 30;
my $DEFAULT_align = "left";
my $DEFAULT_valign = "top";

#
# This routine implements a tabbed properties box.
#
sub do_tabbed
{
my $tabbed_table = shift;		# table of tabbed pages
my $title = shift;				# title for window
my $load_function = shift;		# function to load properties
my $save_function = shift;		# function to save properties
my $max_per = shift;			# maximum number of tabs to show at one time
my $options = shift;

$options->{height} = 425 if(!defined($options->{height}));

# Initialize the internationalization libraries and determine the
# content language and charset.
my ($charset, $content_language) = cgi_intl_init();

# Set $bottom to the bottom button that was pressed.
# This will be empty if a tab or some other button
# was pressed.
my $bottom = cgi_data_move("tab_bottom", "");

# Here we catch Close, Cancel, stuff like that
# This shouldn't happen if Javascript is working.
if($bottom eq 'Cancel' || $bottom eq 'Close')
	{
	require 'cgi_back.pl';
	cgi_back_doit();
	return;
	}

# If the load function hasn't been run yet, do it now.
if(!defined($data{tab_data_loaded}))
	{
	print STDERR "Calling load function...\n" if($debug);
	eval { &$load_function };
	if($@)
		{
		my $message = html($@);
		require "cgi_error.pl";
		error_doc("Load Failed", "<p>" . H_("The load function for this dialog failed with the following error message:") . "<br>\n$message</p>\n");
		return;
		}
	$data{tab_data_loaded} = 1;
	}

# The Save button requires user authentication,
if($bottom eq "Save" && $ENV{REMOTE_USER} eq "")
	{
	require "cgi_auth.pl";
	demand_authentication();
	return;
	}

# Start the HTML document.
#
# We pull in a Cascading Style Sheet and then override some of it in
# Netscape Navigator 4.x.
print <<"DocStart";
Content-Type: text/html;charset=$charset
Content-Language: $content_language
Vary: accept-language

<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd">
<html>
<head>
<title>$title</title>
<meta http-equiv="Content-Style-Type" content="text/css">
<link rel="stylesheet" href="../style/shared.css" type="text/css">
<link rel="stylesheet" href="../style/cgi_tabbed.css" type="text/css">
<style type="text/javascript">
// Fix for broken CSS in Netscape 4.x.
var browser_version = parseFloat(navigator.appVersion);
if(browser_version >= 4.0 && browser_version < 5.0
				&& navigator.appName.indexOf("Microsoft") == -1)
		{
		classes.tabactive.INPUT.borderWidths("0");
		classes.tabactive.INPUT.color = "blue";
		classes.tabinactive.INPUT.borderWidths("0");
		classes.value.SPAN.borderWidths("0");
		classes.value.SPAN.paddings("0");
		}
</style>
</head>
<body>
<form action=\"$ENV{SCRIPT_NAME}\" method="POST">
DocStart

# Figure out which tab is currently selected.
my $page = 0;
my $tab = &cgi_data_move("tab_tab", undef);
my $prev_hscroll = &cgi_data_move('tab_hscroll', 0);
my $hscroll = $prev_hscroll;
if(defined($tab))		# if one was pressed,
	{
	if($tab eq '<<More')
		{
		$page = ($hscroll - 1);					# one in the "<<More" position is selected
		$hscroll = ($page - $max_per + 2);		# scroll back to make it rightmost with ">>More" present
		if($hscroll > 0) { $hscroll++ }			# if room needed for "More>>" scroll right one space
		die if($hscroll < 0);
		}
	elsif($tab eq 'More>>')
		{
		$page = ($hscroll + $max_per - 1);		# one in the "More>>" position is selected
		if($hscroll > 0) { $page-- }			# if there is a "<<More", then we over estimated by one
		$hscroll = $page;						# scroll so selected page is at left
		}
	else
		{
		my $x;
		for($x=0; $x <= $#$tabbed_table; $x++)
			{
			if($tabbed_table->[$x]->{tabname} eq $tab)
				{
				$page = $x;
				last;
				}
			}
		}
	}
else	# if no tab pressed,
	{
	if(defined($data{tab_prevpage}))
		{
		$page = $data{tab_prevpage};
		}
	}

# If there was a previous page, make sure it was left
# in a valid state.
my $error = undef;
{
my $prevpage = $data{tab_prevpage};
if(defined($prevpage))
	{
	my $prevpage_validate = $tabbed_table->[$prevpage]->{onleave};
	if(defined($prevpage_validate))
		{
		print STDERR "Calling onleave function...\n" if($debug);
		if(defined($error = &$prevpage_validate))
			{
			$page = $prevpage;
			$hscroll = $prev_hscroll;
			$bottom = "";
			}
		}
	}
}
$data{tab_prevpage} = $page;
$data{tab_hscroll} = $hscroll;

# The save window doesn't have tabs.  We just insert a spacer
# image to take up their space.
if($bottom eq 'Save')
	{
	print "<img src=\"../images/pixel-clear.png\" alt=\"\" height=20 width=1 border=0>\n";
	}

# Otherwise, we are doing the tabbed thing.
# Print all of the tabs while highlighting
# the selected one.
else
	{
	my $tablast = ($#$tabbed_table + 1);

	my $tabstop = ($hscroll + $max_per);
	if($hscroll > 0) { $tabstop-- }						# if space needed for "<<More"
	if($tabstop < $tablast) { $tabstop-- }				# if space needed for "More>>"
	if($tabstop > $tablast) { $tabstop = $tablast }		# don't go beyond what we have

	#print "<p>\$hscroll = $hscroll, \$page = $page, \$tabstop = $tabstop, \$tablast = $tablast</p>\n";

	if($hscroll > 0)
		{
		isubmit("tab_tab", "<<More", N_("<<More"), _("Bring tabs on the left into view."), undef, "tabinactive");
		}

	my $x = 0;
	foreach my $i (@$tabbed_table)
		{
		if($x >= $hscroll && $x < $tabstop)		# if visible
			{
			my $tabname = $i->{tabname};
			my $class;

			if($x == $page)
				{ $class = "tabactive" }
			else
				{ $class = "tabinactive" }

			# Create a copy of the tabname with the accesskey marker removed.
			(my $tabname_stript = $tabname) =~ s/_//;

			isubmit("tab_tab", $tabname_stript, N_($tabname), undef, undef, $class);
			}
		$x++;
		}

	if($tabstop < $tablast )
		{
		isubmit("tab_tab", "More>>", N_("More>>"), _("Bring tabs on the right into view."), undef, "tabinactive");
		}
	}

# Some pages will override columns or cellpadding.
my $cellpadding = undef;
my $align = undef;
my $valign = undef;
my $help = undef;
if($bottom ne "Save")
	{
	$cellpadding = $tabbed_table->[$page]->{cellpadding};
	$align = $tabbed_table->[$page]->{align};
	$valign = $tabbed_table->[$page]->{valign};
	$help = $tabbed_table->[$page]->{help};
	}
$cellpadding = $DEFAULT_cellpadding if(!defined($cellpadding));
$align = $DEFAULT_align if(!defined($align));
$valign = $DEFAULT_valign if(!defined($valign));

# Start a table.  This table serves as a frame to contain
# the text of the selected tab.
{
#my $spacer_height = $options->{height} - (2 * ($cellpadding - $DEFAULT_cellpadding));
my $spacer_height = $options->{height};
print <<"tableStart";
<table class="tabpage" border=0 cellspacing=0 height=80% width=100%>
<tr align=$align valign=$valign>
<td width="1">
<img src="../images/pixel-clear.png" alt="" width="1" height="$spacer_height" border=0>
</td>
<td style="padding: ${cellpadding}px;">
tableStart
}

# Run the code to generate the current page.
if($bottom eq "Save")
	{
	print STDERR "Calling save function...\n" if($debug);
	eval { &$save_function };
	if($@)
		{
		print "<p>Save failed: ", html($@), "</p>\n";
		}
	}
else
	{
	my $dopage = $tabbed_table->[$page]->{dopage};
	print STDERR "Calling dopage function...\n" if($debug);
	eval { &$dopage };
	if($@)
		{
		print "<p>", html($@), "</p>\n";
		}
	}

# Emmit HTML to end the table.
print <<"TableEnd1";
</td>
</tr>
</table>
TableEnd1

print <<"StartBottomTable";
<table border=0 width="100%" cols=4 cellpadding=5>
<tr>
<td colspan=3>
StartBottomTable

# If there was an error message, print it now.
if(defined($error))
	{ print "<span class=\"alert\">", html($error), "</span>\n" }

print <<"BottomTable2";
</td>
<td nowrap align="right">
<!-- This is for IE bug -->
<input type="image" border="0" name="buggy" src="../images/pixel-clear.png" alt="">
BottomTable2

# Print the bottom buttons.
if($bottom eq 'Save')
	{
	isubmit("tab_bottom", "Close", N_("_Close"), _("Close this window."), "self.close()");
	}
else
	{
	if(defined $help)
		{
		require "cgi_widgets.pl";
		help_button("../help/", $help);
		}
	isubmit("tab_bottom", "Cancel", N_("_Cancel"), _("Close without saving changes."), "self.close()");
	isubmit("tab_bottom", "Save", N_("_Save"), _("Save changes."));
	}

print <<"TableEnd10";
</td>
</tr>
</table>
TableEnd10

# Snap the window size to fit snugly around the document.
if(!cgi_data_peek("resized", 0))
{
print <<"Tail05";
<script>
if(document.width)
		{
		window.resizeTo(document.width + 20, document.height + 20);
		}
</script>
Tail05
$data{resized} = 1;
}

# Emmit the data gathered on other pages as hidden
# form fields.
&cgi_write_data();

# If debugging mode is turned on, then emit this data again
# so that humans can read it easily.  It will normally be
# off the bottom of the page so the user won't see it
# unless he scrolls down.
&cgi_debug_data() if($debug);

# Emmit HTML to end the document
print <<"Tail10";
</form>
</body>
</html>
Tail10
}

1;

