#! /usr/bin/perl -wT
#
# mouse:~ppr/src/www/show_queues.cgi.perl
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
# Last modified 11 December 2003.
#

use 5.005;
use lib "?";
use strict;
use vars qw{%data};
require 'paths.ph';
require 'cgi_data.pl';
require 'cgi_back.pl';
require 'cgi_intl.pl';
require 'cgi_time.pl';
require 'cgi_user_agent.pl';
use PPR::PPOP;

# Spacing
my $CELLPADDING = 5;

# How often to reload page.
my $DEFAULT_REFRESH_INTERVAL = 60;
my $MIN_REFRESH_INTERVAL = 3;

# Icons for representing printer states.
my $Q_ICONS = "../q_icons";
my $Q_ICONS_EXT = ".png";
my $Q_ICONS_DIMS = "width=88 height=88";

# Icon for the fixed things.
my $ICON_ALL_QUEUES = "src=\"../q_icons/10000.png\" alt=\"[group]\" width=88 height=88";
my $ICON_ADD_PRINTER = "src=\"../q_icons/00000.png\" alt=\"[printer]\" width=88 height=88";
my $ICON_ADD_GROUP = "src=\"../q_icons/10000.png\" alt=\"[group]\" width=88 height=88";
my $ICON_ADD_ALIAS = "src=\"../q_icons/20000.png\" alt=\"[alias]\" width=88 height=88";

#
# Routine to convert printer messages to icon selection characters.
#
# The first parameter is a reference to the list of auxiliary status messages
# from the printer.
#
# The second is the character that should be returned if the status messages
# don't contain anything noteworthy.
#
# The third is the character that should be returned if the messages indicate
# that the printer is off-line.
#
# The fourth is the character that should be returned if the printer is in some
# other error state.
#
sub pstatus_char
	{
	my($messages, $default_char, $offline_char, $error_char) = @_;
	foreach my $message (@$messages)
		{
		return $offline_char if($message =~ /^status: PrinterError: off ?line/);
		return $offline_char if($message =~ /^pjl: OFFLINE/);
		return $error_char if($message =~ /^status: PrinterError:/);
		}
	return $default_char;
	}

# Initialize the internationalization libraries and determine the
# content language and charset.
my ($charset, $content_language) = cgi_intl_init();

# Parse the CGI variables.
&cgi_read_data();

# What did the user ask us to do?
my $action = cgi_data_move('action', '');

# Did the user press a "Back" or "Close" button?
if($action eq "Back" || $action eq "Close")
	{
	cgi_back_doit();
	exit 0;
	}

# What back information should we pass to scripts to which we link?
my $encoded_back_stack = cgi_back_init();

# These will become hidden fields which will be used to preserve
# the scroll position across refreshes.	 The "seq" is incremented
# by gentle_reload() (in show_queues.js) so that the browser won't
# use a cached page.  (We send an "Expires:" header, so the pages
# that we generate are cachable.)
if(!defined($data{x})) { $data{x} = 0 }
if(!defined($data{y})) { $data{y} = 0 }
if(!defined($data{seq})) { $data{seq} = 0 }

# In how many columns should the icons be arranged?	 If 0, then
# the comment for each queue is also displayed.	 If -1, then
# the icons will be flowed into the window.
my $columns = cgi_data_move('columns', 6);

# Figure out the refresh interval.
my $refresh_interval = cgi_data_move('refresh_interval', '');
$refresh_interval =~ s/\s*(\d+).*$/$1/;
$refresh_interval = $DEFAULT_REFRESH_INTERVAL if($refresh_interval eq '');
$refresh_interval = $MIN_REFRESH_INTERVAL if($refresh_interval < $MIN_REFRESH_INTERVAL);

#
# Use the USER_AGENT header to decide which features to try to use.
#
my $table_border = 0;
my $fixed_body_style = "";
my $fixed_div_style = "";
{
my $user_agent = cgi_user_agent();

# If CSS level 3 is supported, make the top bar non-scrolling.
if($user_agent->{css3})
	{
	$fixed_body_style = "margin-top: 2em";
	$fixed_div_style = "position:fixed; top:0; left: 0;";
	}

# If images aren't supported, turn on table border so that the text will
# be grouped in some way.
if(!$user_agent->{images})
	{
	$table_border = 1;
	}
}

# We need an informative title.
my $title = html(sprintf(_("PPR Print Queues on \"%s\""), $ENV{SERVER_NAME}));

# Say when this page will expire.  We don't want it to expire
# before the reload, otherwise Netscape may refuse to redraw
# the page if it is resized.
print "Expires: ", cgi_time_format(time() + $refresh_interval + 10), "\n";

# Start the HTML document.
print <<"Head10";
Content-Type: text/html;charset=$charset
Content-Language: $content_language
Vary: user-agent, accept-language

<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">
<html>
<head>
<title>$title</title>
<meta http-equiv="Content-Script-Type" content="text/javascript">
<script type="text/javascript" src="../js/show_queues.js" defer></script>
<link rel="stylesheet" href="../style/show_queues.css" type="text/css">
<link rel="icon" href="../images/icon-16.png" type="image/png">
<link rel="SHORTCUT ICON" href="../images/icon-16.ico">
</head>
<body style="$fixed_body_style" onload="window.scrollTo(document.forms[0].x.value, document.forms[0].y.value)">
<form method="post" action="$ENV{SCRIPT_NAME}">
Head10

# Begin an exception handling block
eval {

#=============================================================================
# Print a lame menu bar.
#=============================================================================

{
print <<"Top5";
<div class="menubar" style="$fixed_div_style">
<label title=${\html_value(_("Choose the display format."))}>
<span class="label">${\H_NB_("View:")}</span>&nbsp;<select name="columns" onchange="document.forms[0].submit()">
Top5

print "<option";
print " selected" if($columns == 0);
print " value=\"0\">", H_("Free Flow"), "\n";

print "<option";
print " selected" if($columns == -1);
print " value=\"-1\">", H_("Details"), "\n";

print "<option";
print " selected" if($columns == -2);
print " value=\"-2\">", H_("Many Details"), "\n";

for(my $i = 4; $i <= 24; $i += 2)
	{
	print "<option";
	print " selected" if($columns == $i);
	print " value=\"$i\">", html(sprintf(_("%d Columns"), $i)), "\n";
	}

print <<"Top7";
</select>
</label>
Top7

# Add a control for the refresh interval.
print <<"Top8";
<label title=${\html_value(_("This page will be reloaded at the indicated interval (in seconds)."))}>
<span class="label">${\H_NB_("Refresh Interval:")}</span>&nbsp;<input type="text" name="refresh_interval" value="$refresh_interval" size=4 onchange="document.forms[0].submit()">
</label>
Top8

# Add a refresh button.
isubmit("action", "Refresh", N_("Refresh"), 'class="buttons" onclick="gentle_reload(); return false"', _("Refresh the page right now."));

# Create a button for JavaScript Cookie login.
ibutton(_("Cookie Login"), "window.open('../html/login_cookie.html', '_blank', 'width=350,height=250,resizable')", "class=\"buttons\"", _("Use this if your browser doesn't support Digest authentication."));

# Try to make a "Close" button.	 (They do the same thing.)
print "<label title=", html_value(_("Close this window.")), ">\n";
isubmit("action", "Close", N_("_Close"), "class=\"buttons\" onclick=\"window.close(); return false;\"");
print "</label>\n";

print <<"Top10";
</div>
Top10
}

#=============================================================================
# Here we create the icons for the wizards and for Show All Jobs.
#=============================================================================

print <<"Top10";
<table align="left" border=$table_border cellspacing=0 cellpadding=$CELLPADDING width="50%">
<tr align=center>
Top10

foreach my $i (
	[$ICON_ALL_QUEUES,
		_("Show All Queues"),
		"show_jobs.cgi?name=all;",
		"return show_jobs(null,this.href)",
		_("Click here to open a window which will show all queued jobs.")
		],
	[$ICON_ADD_PRINTER,
		_("Add New Printer"),
		"prn_addwiz.cgi?",
		"return wizard(null,this.href)",
		_("Click here and you will be guided through the process of adding a new printer.")
		],
	[$ICON_ADD_GROUP,
		_("Add New Group"),
		"grp_addwiz.cgi?",
		"return wizard(null,this.href)",
		_("Click here and you will be guided through the process of adding a new group of printers.")
		],
	[$ICON_ADD_ALIAS,
		_("Add New Alias"),
		"alias_addwiz.cgi?",
		"return wizard(null,this.href)",
		_("Click here and you will be guided through the process of adding a new alias.")
		]
	)
{
print <<"Top20";
<td width="25%"><a
	href="$i->[2]$encoded_back_stack"
	onclick="$i->[3]"
	title=${\html_value($i->[4])}
	target="_blank"
	>
	<img $i->[0] border=0><br>
	<span class="qname">${\html($i->[1])}</span>
	</a></td>
Top20
}

print <<"Top100";
</tr>
</table>
<br clear="left">
<hr>
Top100

#=============================================================================
# Here we loop through the available print destinations, gathering information
# about type, status, number of jobs, etc. and choosing an icon for each.
#
# The information is stored in $queues{} and $queues_counts{}.
#=============================================================================

my %queues;
my %queues_counts;
{
my $control = new PPR::PPOP("all");

# Select the correct member function to get the level of detail we need.
#my $function;
#if($columns >= 0)
#	 { $function = "list_destinations" }
#elsif($columns == -1)
#	 { $function = "list_destinations_comments" }
#else
#	 { $function = "list_destinations_comments_addresses" }

# Get a list of all queues with a description for each.
foreach my $row ($control->list_destinations_comments_addresses())
	{
	my($name, $type, $accepting, $protected, $comment, $address) = @$row;
	my $icon = '';

	if($type eq 'printer')
		{ $icon .= "0" }
	elsif($type eq 'group')
		{ $icon .= "1" }
	elsif($type eq 'alias')
		{ $icon .= "2" }
	else
		{ $icon .= "x" }

	if($accepting eq 'accepting')
		{ $icon .= '0' }
	elsif($accepting eq 'rejecting')
		{ $icon .= '1' }
	elsif($accepting eq '?')
		{ $icon .= '0' }
	else
		{ $icon .= 'x' }

	if($protected eq 'no')
		{ $icon .= '0' }
	elsif($protected eq 'yes')
		{ $icon .= '1' }
	elsif($protected eq '?')
		{ $icon .= '0' }
	else
		{ $icon .= 'x' }

	$queues{$name} = [$type, $icon, $comment, $address];
	$queues_counts{$name} = 0;
	}

# Count the number of jobs in each queue.  The counts are
# stored in $queues_count{}.
foreach my $row ($control->qquery('destname'))
	{
	$queues_counts{$row->[0]}++;
	}

# Add the correct "jobs present" (0 or 1) character for each queue.
foreach my $qname (keys(%queues))
	{
	if($queues_counts{$qname} > 0)
		{
		$queues{$qname}->[1] .= '1';
		}
	else
		{
		$queues{$qname}->[1] .= '0';
		}
	}

#
# Refine our icon selection for destinations that are printers by
# adding an additional characters that indicates the printer status.
#
# These are the available characters:
#
# 0		idle
# 1		printing
# 2		stopping
# 3		canceling job
# 4		stopt
# 5		fault
# 6		fault no-auto-retry
# 7		engaged
# 8		off-line
# 9		other "PrinterError:"
# a		printing but off-line
# b		printing but other "PrinterError:"
#
foreach my $row ($control->get_pstatus())
	{
	my $name = shift @$row;
	my $status = shift @$row;
	my $job = shift @$row;
	my $retry_number = shift @$row;
	my $retry_countdown = shift @$row;
	my @messages = @$row;
	#print STDERR "\$name=\"$name\" \$status=\"$status\" \$job=\"$job\" \$retry_number=$retry_number \$retry_countdown=$retry_countdown \@messages=(\"", join('", "', @messages), "\")\n";

	my($type, $icon, $comment, $qaddress) = @{$queues{$name}};
	#print STDERR "\$type=\"$type\", \$icon=\"$icon\", \$comment=\"$comment\", \$qaddress=\"$qaddress\"\n";

	if($status eq "idle")
		{ $icon .= pstatus_char(\@messages, '0', '8', '9') }
	elsif($status eq "printing")
		{ $icon .= pstatus_char(\@messages, '1', 'a', 'b') }
	elsif($status eq "stopping" || $status eq "halting")
		{ $icon .= '2' }
	elsif($status eq "canceling" || $status eq "seizing")
		{ $icon .= '3' }
	elsif($status eq "stopt")
		{ $icon .= '4' }
	elsif($status eq "fault")
		{
		if($retry_number > 0)			# retries
			{ $icon .= '5' }
		else							# no retries
			{ $icon .= '6' }
		}
	elsif($status eq "starved")
		{
		$icon .= "5";
		}
	elsif($status eq "engaged")
		{
		$icon .= pstatus_char(\@messages, '7', '8', '9')
		}
	else
		{ $icon .= 'x' }

	$queues{$name} = [$type, $icon, $comment, $qaddress];
	}

# We have all the information we need, shut down ppop.
$control->destroy;
}

#=============================================================================
# Produce the HTML for the print destination icons.
#=============================================================================

# If there is going to be only a single table, start it.
if($columns != 0)
	{ print "<table border=$table_border cellspacing=0 cellpadding=$CELLPADDING>\n" }

# Loop thru the queues, creating a labeled icon for each.
# Code needed to escape the queue name in HTML, URLs and JavaScript!
{
my $col = 0;
foreach my $qname (sort(keys(%queues)))
	{
	my($qtype, $icon, $qdescription, $qaddress) = @{$queues{$qname}};

	# Group and alias icon basenames are still missing the last zero.
	$icon .= '0' if(length($icon) < 5);

	# Some queues will have no description because the system
	# administrator was lazy.
	if($qdescription eq '') { $qdescription = '-- No Description --' }

	# Define these here to avoid code duplication.
	my $a_tag = "<a
		href=\"#" . $qtype . "_" . $qname . "\"
		title=\"" . html($qdescription) . "\"
		onclick=\"return popup(event," . javascript_string($qtype . "_" . $qname) . ")\" 
		oncontextmenu=\"return popup(event," . javascript_string($qtype . "_" . $qname) . ")\" 
		target=\"_blank\">";
	my $img_tag = "<img src=\"$Q_ICONS/$icon$Q_ICONS_EXT\" $Q_ICONS_DIMS alt=\"[$qtype]\" border=0>";
	my $jcount = ($queues_counts{$qname} > 0 ? "<span class=\"qjobs\">($queues_counts{$qname})</span>" : "");

	if($columns > 0)			# multicolumn or single column w/out details
		{
		print "<tr align=center>\n" if(($col % $columns) == 0);
		print "<td>$a_tag$img_tag$jcount<br><span class=\"qname\">", html($qname), "</span></a></td>\n";
		$col++;
		print "</tr>\n" if(($col % $columns) == 0);
		}
	elsif($columns == 0)		# free flow
		{
		print "<table align=\"left\" border=$table_border cellspacing=0 cellpadding=$CELLPADDING>";
		print "<tr align=\"center\">";
		print "<td>$a_tag$img_tag$jcount<br><span class=\"qname\">", html($qname), "</span></a></td>";
		print "</tr></table>\n";
		}
	else						# Details (-1) or Many Details (-2)
		{
		print "<tr align=center><td>$a_tag$img_tag$jcount<br><span class=\"qname\">", html($qname), "</span></a></td>";
		print "<td align=\"left\"><span class=\"qcomment\">", html($qdescription), "</span>";
		print "<br><span class=\"qaddress\">", html($qaddress), "</span>" if($columns == -2);
		print "</td></tr>\n";
		}

	}

# If there was a single table, close any dangling row and
# then close the table.
if($columns != 0)
	{
	print "</tr>\n" if($columns > 0 && $col % $columns);
	print "</table>\n";
	}
print "\n";
}

#=============================================================================
# Emit the HTML for the popup menus.
#=============================================================================
foreach my $qname (sort(keys(%queues)))
{
my($qtype, $icon, $qdescription, $qaddress) = @{$queues{$qname}};

my $encoded_name = form_urlencoded("name", $qname);
my $encoded_type = form_urlencoded("type", $qtype);
my $encoded_tag = "${qtype}_${qname}";
my $js_name = javascript_string($qname);

print <<"POP";
<div class="popup" id="$encoded_tag" onmouseover="offmenu(event)">
<a name="$encoded_tag">
<table border="1" cellspacing="0" class="menu">
<noscript>
<tr><th>${\html("Menu for $qtype $qname")}</th></tr>
<tr><td><a href=\"$ENV{SCRIPT_NAME}\">Back to Top</a></td></tr>
</noscript>
POP

if($qtype eq "printer")
{
print <<"Printer";
<tr><td><a href="show_jobs.cgi?$encoded_name;$encoded_back_stack"
	onclick="return show_jobs(event,this.href)" 
	>${\H_("View Queue")}</a></td></tr>
<tr><td><a href="prn_control.cgi?$encoded_name;$encoded_back_stack"
	onclick="return prn_control(event,$js_name)" 
	>${\H_("Printer Control")}</a></td></tr>
<tr><td><a href="prn_properties.cgi?$encoded_name;$encoded_back_stack"
	onclick="return properties(event,this.href)" 
	>${\H_("Printer Properties")}</a></td></tr>
<tr><td><a href="prn_testpage.cgi?$encoded_name;$encoded_back_stack"
	onclick="return wizard(event,this.href)" 
	>${\H_("Test Page")}</a></td></tr>
<tr><td><a href="cliconf.cgi?$encoded_name;$encoded_back_stack"
	onclick="return wizard(event,this.href)" 
	>${\H_("Client Configuration")}</a></td></tr>
<tr><td><a href="show_printlog.cgi?${\form_urlencoded("printer", $qname)};$encoded_back_stack"
	onclick="return show_printlog(event,'printer',$js_name)" 
	>${\H_("Printlog")}</a></td></tr>
<tr><td><a href="delete_queue.cgi?$encoded_type&$encoded_name;$encoded_back_stack"
	onclick="return confirm(event,this.href)" 
	>${\H_("Delete Printer")}</a></td></tr>
Printer
}

elsif($qtype eq "group")
{
print <<"Group";
<tr><td><a href="show_jobs.cgi?$encoded_name;$encoded_back_stack"
	onclick="return show_jobs(event,this.href)" 
	>${\H_("View Queue")}</a></td></tr>
<tr><td><a href="grp_control.cgi?$encoded_name;$encoded_back_stack"
	onclick="return grp_control(event,$js_name)" 
	>${\H_("Member Printer Control")}</a></td></tr>
<tr><td><a href="grp_properties.cgi?$encoded_name;$encoded_back_stack"
	onclick="return properties(event,this.href)" 
	>${\H_("Group Properties")}</a></td></tr>
<tr><td><a href="cliconf.cgi?$encoded_name;$encoded_back_stack"
	onclick="return wizard(event,this.href)" 
	>${\H_("Client Configuration")}</a></td></tr>
<tr><td><a href="show_printlog.cgi?${\form_urlencoded("queue", $qname)};$encoded_back_stack"
	onclick="return show_printlog(event,'group',$js_name)" 
	>${\H_("Printlog")}</a></td></tr>
<tr><td><a href="delete_queue.cgi?$encoded_type&$encoded_name;$encoded_back_stack"
	onclick="return confirm(event,this.href)" 
	>${\H_("Delete Group")}</a></td></tr>
Group
}

elsif($qtype eq "alias")
{
print <<"Alias";
<tr><td><a href="alias_properties.cgi?$encoded_name;$encoded_back_stack"
	onclick="return properties(event,this.href)" 
	>${\H_("Alias Properties")}</a></td></tr>
<tr><td><a href="cliconf.cgi?$encoded_name;$encoded_back_stack"
	onclick="return wizard(event,this.href)" 
	>${\H_("Client Configuration")}</a></td></tr>
<tr><td><a href="delete_queue.cgi?$encoded_type;$encoded_name;$encoded_back_stack"
	onclick="return confirm(event,this.href)" 
	>${\H_("Delete Alias")}</a></td></tr>
Alias
}

else
{
die;
}

print "</table>\n";
print "</div>\n";
print "\n";
}

#=============================================================================
# Final stuff
#=============================================================================

# End the exception handling block.
}; if($@)
	{
	print "<p>$@</p>\n";
	}

# Emmit a form which will contain any values we want to send to the
# next invokation.
cgi_write_data();

print <<"Tail50";
</form>
<script>
window.setTimeout("gentle_reload()", $refresh_interval * 1000);
</script>
</body>
</html>
Tail50

exit 0;

