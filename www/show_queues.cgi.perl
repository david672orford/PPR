#! /usr/bin/perl -wT
#
# mouse:~ppr/src/www/show_queues.cgi.perl
# Copyright 1995--2001, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is" without
# express or implied warranty.
#
# Last modified 2 August 2001.
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
#my $ICON_ADD_GROUP = "../images/group.png";
#my $ICON_ADD_PRINTER = "../images/printer.png";
#my $ICON_ALL_QUEUES = "../images/group.png";
my $ICON_ADD_PRINTER = "src=\"../q_icons/00000.png\" alt=\"[printer]\" width=85 height=75";
my $ICON_ADD_GROUP = "src=\"../q_icons/10000.png\" alt=\"[group]\" width=85 height=75";
my $ICON_ALL_QUEUES = "src=\"../q_icons/10000.png\" alt=\"[group]\" width=85 height=75";

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

# What back information should we pass to scripts we link to?
my $encoded_back_stack = cgi_back_stackme();

# These will become hidden fields which will be used to preserve
# the scroll position across refreshes.  The "seq" is incremented
# by gentle_reload() (in show_queues.js) so that the browser won't
# use a cached page.  (We send an "Expires:" header, so the pages
# that we generate are cachable.)
if(!defined($data{x})) { $data{x} = 0 }
if(!defined($data{y})) { $data{y} = 0 }
if(!defined($data{seq})) { $data{seq} = 0 }

# In how many columns should the icons be arranged?  If 0, then
# the comment for each queue is also displayed.  If -1, then
# the icons will be flowed into the window.
my $columns = cgi_data_move('columns', 6);

# Figure out the refresh interval.
my $refresh_interval = cgi_data_move('refresh_interval', '');
$refresh_interval =~ s/\s*(\d+).*$/$1/;
$refresh_interval = $DEFAULT_REFRESH_INTERVAL if($refresh_interval eq '');
$refresh_interval = $MIN_REFRESH_INTERVAL if($refresh_interval < $MIN_REFRESH_INTERVAL);

# Turn on table borders unless we think the web browser is likely to
# be displaying images.  Because of this code, we must emit
# a "Vary: user-agent" header.
my $table_border = 1;
if(defined($ENV{HTTP_USER_AGENT}) && $ENV{HTTP_USER_AGENT} =~ /^Mozilla\//)
    {
    if(!cgi_data_peek("borders", "0"))
	{ $table_border = 0 }
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

<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd">
<html>
<head>
<title>$title</title>
<meta http-equiv="Content-Script-Type" content="text/javascript">
<script type="text/javascript" src="../js/show_queues.js" defer></script>
<link rel="stylesheet" href="../style/show_queues.css" type="text/css">
</head>
<body onload="window.scrollTo(document.forms[0].x.value, document.forms[0].y.value)">
<div id="popup" class="menu">
This text is supposed to be hidden.
</div>
<form method="post" action="$ENV{SCRIPT_NAME}">
Head10

# Begin an exception handling block
eval {

#=============================================================================
# Print a lame menu bar.
#=============================================================================

# IE 5.0 choaks on this:
#<div style="position:fixed; top:0; left: 0; width:100%; height: 2em; background:white">
{
print <<"Top5";
<div class="menubar">
<label><span class="label">${\H_("View:")}</span>&nbsp;
<select name="columns" onchange="document.forms[0].submit()")>
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
<spacer type="horizontal" size=10>
<label><span class="label">${\H_("Refresh Interval:")}</span>
<input type="text" name="refresh_interval" value="$refresh_interval" size=4 onchange="document.forms[0].submit()">
</label>
Top8

# Add a refresh button.
print <<"Top9";
<spacer type="horizontal" size=50>
<input class="buttons" type="submit" name="action" value="Refresh" onclick="gentle_reload(); return false">
Top9

# Create a button for JavaScript Cookie login.
print '<spacer type="horizontal" size=10>', "\n";
print '<input type="button" value=', html_value(_("Cookie Login")), ' onclick="';
print "window.open('../html/login_cookie.html', '_blank', 'width=350,height=250,resizable')";
print '">', "\n";

# Try to make a "Close" button.  (They do the same thing.)
cgi_back_possible("/");
print '<spacer type="horizontal" size=10>', "\n";
isubmit("action", "Close", N_("_Close"), "class=\"buttons\" onclick=\"window.close(); return false;\"");

print <<"Top10";
</div>
Top10
}

#=============================================================================
# Here we create the icons for the wizards and for Show All Jobs.
#=============================================================================

print <<"Top10";
<table border=$table_border cellspacing=0 cellpadding=$CELLPADDING>
<tr align=center>
<td><a href="prn_addwiz.cgi?$encoded_back_stack" onclick="return wizard('prn_addwiz.cgi')">
	<img $ICON_ADD_PRINTER border=0><br>
	<span class="qname">${\H_("Add New Printer")}</span>
        </a></td>
<td><a href="grp_addwiz.cgi?$encoded_back_stack" onclick="return wizard('grp_addwiz.cgi')">
	<img $ICON_ADD_GROUP border=0><br>
	<span class="qname">${\H_("Add New Group")}</span>
	</a></td>
<td><a href="show_jobs.cgi?name=all;$encoded_back_stack" onclick="show_jobs('all'); return false">
	<img $ICON_ALL_QUEUES border=0><br>
	<span class="qname">${\H_("Show All Queues")}</span>
	</a></td>
</tr>
</table>
<hr>
Top10

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
#    { $function = "list_destinations" }
#elsif($columns == -1)
#    { $function = "list_destinations_comments" }
#else
#    { $function = "list_destinations_comments_addresses" }

# Get a list of all queues with a description for each.
print STDERR "xxxxxx\n";
print STDERR "\$control = $control\n";
foreach my $row ($control->list_destinations_comments_addresses())
    {
    my($name, $type, $accepting, $protected, $comment, $address) = @$row;
print STDERR "\$name = \"$name\"\n";
    my $icon = '';

    if($type eq 'printer')
    	{ $icon .= "0" }
    elsif($type eq 'group')
    	{ $icon .= "1" }
    else
    	{ $icon .= "x" }

    if($accepting eq 'accepting')
    	{ $icon .= '0' }
    elsif($accepting eq 'rejecting')
    	{ $icon .= '1' }
    else
    	{ $icon .= 'x' }

    if($protected eq 'yes')
    	{ $icon .= '1' }
    elsif($protected eq 'no')
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
# 0	idle
# 1	printing
# 2	stopping
# 3	canceling job
# 4	stopt
# 5	fault
# 6	fault no-auto-retry
# 7	engaged
# 8	off-line
# 9	other "PrinterError:"
# a	printing but off-line
# b	printing but other "PrinterError:"
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
	if($retry_number > 0)		# retries
	    { $icon .= '5' }
	else				# no retries
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
# Shut down ppop.
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

    # Group icon basenames are still missing the last zero.
    $icon .= '0' if(length($icon) < 5);

    # Some queues will have no description because the system
    # administrator was lazy.
    if($qdescription eq '') { $qdescription = '-- No Description --' }

    # Define these here to avoid code duplication.
    my $a_tag = "<a href=\"show_queues_nojs.cgi?type=$qtype;" . form_urlencoded("name", $qname) . ";$encoded_back_stack\" onclick=\"return $qtype(event," . javascript_string($qname) . ")\">";
    my $img_tag = "<img src=\"$Q_ICONS/$icon$Q_ICONS_EXT\" $Q_ICONS_DIMS alt=\"[$qtype]\" border=0>";
    my $jcount = ($queues_counts{$qname} > 0 ? "<span class=\"qjobs\">($queues_counts{$qname})</span>" : "");

    if($columns > 0)		# multicolumn or single column w/out details
	{
	print "<tr align=center>\n" if(($col % $columns) == 0);
	print "<td>$a_tag$img_tag$jcount<br><span class=\"qname\">", html($qname), "</span></a></td>\n";
	$col++;
	print "</tr>\n" if(($col % $columns) == 0);
	}
    elsif($columns == 0)	# free flow
        {
	print "<table align=\"left\" border=$table_border cellspacing=0 cellpadding=$CELLPADDING>";
	print "<tr align=\"center\"><td>$a_tag$img_tag$jcount<br><span class=\"qname\">", html($qname), "</span></a></td></tr></table>\n";
        }
    else			# Details (-1) or Many Details (-2)
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

