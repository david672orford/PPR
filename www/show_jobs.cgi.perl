#! /usr/bin/perl -wT
#
# mouse:~ppr/src/www/show_jobs.cgi.perl
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
# Last modified 17 December 2003.
#

use 5.005;
use lib "?";
require 'paths.ph';
require 'cgi_data.pl';
require 'cgi_intl.pl';
require "cgi_widgets.pl";
require 'qquery_xlate.pl';
require 'cgi_user_agent.pl';

defined($CONFDIR) || die;
defined(@qquery_available) || die;
defined(%qquery_xlate) || die;

# Should we dump the variables at the bottom?
my $DEBUG = 0;

# What ppop subcommand should we run on button presses?
my %ACTIONS = (
		'Move' => 'move',
		'Cancel' => 'cancel',
		'Rush' => 'rush',
		'Hold' => 'hold',
		'Release' => 'release',
		'Refresh' => '',
		'Done' => ''
);

# Initialize the internationalization libraries and determine the
# content language and charset.
my ($charset, $content_language) = cgi_intl_init();

# Read in CGI name=value pairs and place them
# in %data.
&cgi_read_data();

# Should we display controls?
# Make a hash which represents the buttons that should be shown.
if(!defined $data{controls})
	{
	$data{controls} = "Queue View Refresh Close Help Move Cancel Rush Hold Release Modify Log";
	}
my %controls;
foreach my $b (split(/ /, $data{controls}))
	{
	$controls{$b} = 1;
	}

my $queue_name = cgi_data_peek("name", "all");

# If the queue type wasn't specified, figure it out.
if(!defined $data{type})
	{
	if(-f "$CONFDIR/aliases/$queue_name")
		{ $data{type} = "alias" }
	elsif(-f "$CONFDIR/groups/$queue_name")
		{ $data{type} = "group" }
	elsif(-f "$CONFDIR/printers/$queue_name")
		{ $data{type} = "printer" }
	else
		{ $data{type} = "all" }
	}
my $queue_type = cgi_data_peek("type", "?");

# These will become hidden fields which will be used to preserve
# the scroll position across refreshes.
if(!defined($data{x})) { $data{x} = 0 }
if(!defined($data{y})) { $data{y} = 0 }
if(!defined($data{seq})) { $data{seq} = 0 }

# How often to reload?
my $refresh_interval = cgi_data_move("refresh_interval", 60);
$refresh_interval =~ s/\s*(\d+).*$/$1/;		# paranoid
$refresh_interval = 3 if($refresh_interval < 3);

# What are we to do?
my $action = cgi_data_move("action", undef);

# Was the form automatically submitted because someone changed the
# "Move To" select?  If so, then the action is "Move".
my $move_to = cgi_data_move("move_to", "");
$action = "Move" if($move_to ne "");

# Get the list of columns to display in the table.
my @fields;
if(defined($data{fields}))
	{
	@fields = split(/ /, cgi_data_peek('fields', undef));
	}
else
	{
	@fields = qw(jobname status/explain subtime pagesxcopies for title creator);
	$data{fields} = join(' ', @fields);
	}

#=============================================================
# Handle lack of JavaScript.
#=============================================================
if(defined($action))
	{
	# These actions load other CGI URLs.
	if($action eq "Modify" || $action eq "Log")
		{
		if(defined($data{jobs}))
			{
			require 'cgi_redirect.pl';
			my $job = (split(/ /, $data{jobs}))[0];
			my $lc_action = $action;
			$lc_action =~ tr/[A-Z]/[a-z]/;
			my $encoded_HIST = form_urlencoded("HIST", $data{HIST});
			my $encoded_jobname = form_urlencoded("jobname", $job);
			cgi_redirect("http://$ENV{SERVER_NAME}:$ENV{SERVER_PORT}/cgi-bin/job_${lc_action}.cgi?$encoded_jobname;$encoded_HIST");
			exit 0;
			}
		}
	# Since JavaScript didn't work to close this window, try to load
	# the previous page.
	elsif($action eq "Close")
		{
		require 'cgi_back.pl';
		cgi_back_doit();
		exit 0;
		}
	}

#=============================================================
# Demand authentication if it is required.	Notice that
# buttons with their action routines defined as an empty
# string do not require authentication.
#=============================================================
if(defined($action))
	{
	if(undef_to_empty($ACTIONS{$action}) ne "" && undef_to_empty($ENV{REMOTE_USER}) eq "")
		{
		require "cgi_auth.pl";
		demand_authentication();
		exit 0;
		}
	}

#=============================================================
# If the "View" button was pressed, we produce the
# HTML document right here in this block.
#=============================================================
if(defined($action) && $action eq 'View')
{
&cgi_data_move('fields', undef);

my $title = html(_("Queue Display Settings"));

print <<"Settings10";
Content-Type: text/html;charset=$charset
Content-Language: $content_language
Vary: accept-language

<html>
<head>
<title>$title</title>
<link rel="stylesheet" href="../style/shared.css" type="text/css">
<link rel="stylesheet" href="../style/show_jobs.css" type="text/css">
</head>
<body>
<form method="POST" action="$ENV{SCRIPT_NAME}">
<p>
Settings10

# Make a hash of the fields that we will be displaying.
my %fields_hash = ();
foreach my $i (@fields)
	{ $fields_hash{$i} = 1 }

# Print a checkbox for each possible column.
foreach my $i (@qquery_available)
	{
	if($i eq "-")
		{
		print "<hr class=\"sep\">\n";
		next;
		}
	my($short, $long) = @{$qquery_xlate{$i}};
	print "<nobr><input type=\"checkbox\" name=\"fields\" value=", html_value($i);
	print " checked" if(defined($fields_hash{$i}));
	print ">", H_($short);
	print " &#151; ", H_($long) if(defined $long);		# em dash
	print "</nobr><br>\n";
	}

print "</p>\n";

# Emmit the hidden fields.
&cgi_write_data();

print <<"Settings90";
<p><input type="submit" name="action" value="Done"></p>
</form>
</body>
</html>
Settings90

exit 0;
}

#=============================================================
# Emmit the start of the document and the start of the form.
#=============================================================

#
# Make descisions based on the browser.	 Because of this code, we must emit
# a "Vary: user-agent" header.
#
# For example, we turn on table borders unless we think the web browser is
# likely be capable of displaying images.
#
my $user_agent = cgi_user_agent();
my $fixed_html_style = "";
my $fixed_div_style_top = "";
my $fixed_div_style_bottom = "";
if($data{controls})
	{
	# If CSS fixed positioning is likely to work, use it for the top bar.
	if($user_agent->{css_fixed})
		{
		$fixed_html_style = "margin-top: 2em";
		$fixed_div_style_top = "position:fixed; top:0pt; left: 0pt;";
		$fixed_div_style_bottom = "position:fixed; bottom: 0pt; left: 0pt;";
		}
	}

my $title = html(sprintf(_("Jobs Queued for \"%s\" on \"%s\""), $queue_name, $ENV{SERVER_NAME}));

#print "Expires: ", cgi_time_format(time() + $refresh_interval + 10), "\n";

print <<"Quote10";
Content-Type: text/html;charset=$charset
Content-Language: $content_language
Vary: user-agent, accept-language

<html style="$fixed_html_style">
<head>
<title>$title</title>
<link rel="stylesheet" href="../style/shared.css" type="text/css">
<link rel="stylesheet" href="../style/show_jobs.css" type="text/css">
<HTA:APPLICATION navigable="yes"></HTA:APPLICATION>
<meta http-equiv="Content-Script-Type" content="text/javascript">
<script type="text/javascript" src="../js/show_queues.js" defer></script>
<script type="text/javascript" src="../js/show_jobs.js" defer></script>
</head>
<body onload="window.scrollTo(document.forms[0].x.value, document.forms[0].y.value)">
<form method="POST" action="$ENV{SCRIPT_NAME}">
Quote10

if($data{controls})
{
print <<"Quote20";
<div class="menubar" style="$fixed_div_style_top">
	<!-- This is for a Netscape 4.x bug: -->
	<input type="image" border="0" name="action" value="Refresh" src="../images/pixel-clear.png" alt="">
Quote20

# New-style menu bar
if($user_agent->{css_dom})
{
my @refresh_values = (
	[$refresh_interval, _("Now")]
	);
foreach my $i (qw(5 10 15 30 45 60 90 120))
	{
	push(@refresh_values, [$i, sprintf(_("Every %d seconds"), $i)]);
	}

menu_start("m_file", _("File"));
		menu_submit("action", "Close", N_("_Close"), _("Close this window."), "window.close()");
menu_end();

menu_start("m_view", _("View"));
	#menu_radio_set("columns", \@col_values, $columns, 'onchange="document.forms[0].submit()"');
	menu_submit("action", "View", _("Preferences"));
menu_end();

menu_start("m_refresh", _("Refresh"));
	menu_radio_set("refresh_interval", \@refresh_values, $refresh_interval, 'onchange="document.forms[0].submit()"');
menu_end();

menu_tools();

menu_window($queue_type, $queue_name);

menu_help();
}

# Old-style menu bar
else
{
isubmit("action", "View", _("View")) if(defined $controls{View});
labeled_entry("refresh_internal", _("Refresh Interval:"), $refresh_interval, 4, 
	_("This page will be reloaded at the indicated interval (in seconds)."),
	'onchange="document.forms[0].submit()"'
	);
isubmit("action", "Refresh", _("Refresh")) if(defined $controls{Refresh});
isubmit("action", "Close", _("Close"), _("Close this window."), "window.close()") if(defined $controls{Close});
print "<span style='margin-left: 9cm'></span>\n";
help_button("../help/", undef) if(defined $controls{Help})
}

print "</div>\n";
}

#=============================================================
# This is the start of an exception handling block:
#=============================================================
eval {
# Pull in the library at run time.
require PPR::PPOP;

# Create the object we use to examine and to manipulate the queue.
my $control = new PPR::PPOP($queue_name);

#----------------------------------------------------------------------------
# If a button was pressed, act on it.
#----------------------------------------------------------------------------
{
if(defined($action))
	{
	my @action_result = ();
	my $action_routine = $ACTIONS{$action};

	# Catch script errors or users playing games.
	if(!defined($action_routine))
		{
		@action_result = ("It seems you pressed a bottom that doesn't exist!",
						"This means either that there is a bug in this",
						"CGI script or you are trying to confuse it",
						"by doing tricky things with your web browser.");
		}

	# If the action for this button is non-empty, invoke it as a method
	# of a PPRcontrol object representing the queue.
	if($action_routine ne "")
		{
		# This should have been caught already.
		die if(undef_to_empty($ENV{REMOTE_USER}) eq "");

		# Become the remote user for PPR purposes.
		require "acl.pl";
		if(defined(getpwnam($ENV{REMOTE_USER})) || user_acl_allows($ENV{REMOTE_USER}, "ppop"))
			{
			$control->su($ENV{REMOTE_USER});
			}
		else
			{
			$control->proxy_for("$ENV{REMOTE_USER}\@*");
			}

		# Fetch the list of jobs to act on.
		my @joblist = split(/ /, cgi_data_move("jobs", ""));

		# If empty list of jobs to action on,
		if((scalar @joblist) < 1)
			{
			push(@action_result, sprintf(_("You must select one or more jobs before pressing [%s]."), $action));
			}

		# If action is to move jobs,
		elsif($action_routine eq 'move')
			{
			if($move_to eq "")
				{
				push(@action_result, sprintf(_("You must select a destination queue before pressing [Move].")));
				}
			else
				{
				foreach my $job (@joblist)
					{
					push(@action_result, $control->move($job, $move_to));
					}
				}
			}

		# If any other action,
		else
			{
			push(@action_result, $control->$action_routine(@joblist));
			}
		}

	# If any of the code above pushed unpleasant news onto the error message
	# list, call the error_window() function to create a window with each list
	# item as a line.  error_window() takes care of HTML() escaping the lines.
	if((scalar @action_result) > 0)
		{
		require 'cgi_error.pl';
		error_window(sprintf(_("[%s] failed for the following reason:"), $action), @action_result);
		}
	}
}

#----------------------------------------------------------------------------
# If we haven't done it already, fetch a list of the available destinations.
#----------------------------------------------------------------------------
if(!defined($data{dests}))
	{
	my @result_rows = $control->list_destinations("all");
	my @dests_list = ();
	foreach my $d (@result_rows)
		{
		push(@dests_list, $d->[0]);
		}
	@dests_list = sort(@dests_list);
	$data{dests} = join(" ", @dests_list);
	}

#=============================================================
# Start the table which holds the queue listing and
# write the column headings.
#=============================================================
print <<"THEAD10";
<div class="queue">
<table class="queue" title="$title" border=1 cellpadding=5 cellspacing=0
summary="This table has one row for each print job.	 The left hand column has
a checkbox for each print job.	Elsewhere on the page there are buttons which
act on all checked jobs.  The other columns display information about each
print job.	There may be blank rows at the end of the table.">
<thead>
THEAD10

# Create the column headings.
print "<tr><th>&nbsp;</th>";
foreach my $i (@fields)
	{ print "<th scope=\"col\">", html($qquery_xlate{$i}->[0]), "</th>" }
print "</tr>\n";

print <<"THEAD20";
</thead>
<tbody>
THEAD20

# Make a hash of the jobs that should be checked.
my %checked_list;
foreach my $i (split(/ /, cgi_data_move('jobs', '')))
	{ $checked_list{$i} = '' }

# Initialize count of rows used.
my $jobcount = 0;

# Get the data.
my @answer = $control->qquery('jobname', @fields);

# Do a row for each print job.
foreach my $i (@answer)
	{
	$jobcount++;
	my @list = @{$i};
	my $jobid = shift(@list);

	# Print a checkbox and make it checked if the job was selected.
	print "<tr id=\"j$jobcount\"";
	print " class=\"checked\"" if(defined $checked_list{$jobid});
	print ">";
	print "<th scope=\"row\" title=", html_value(sprintf(_("Job \"%s\""), $jobid)), ">";
	print "<input type=\"checkbox\" name=\"jobs\" value=\"$jobid\"";
	print " onclick=\"var row=document.getElementById('j$jobcount');if(row.className == '') {row.className ='checked'} else {row.className=''};return true;\"";
	print " checked" if(defined $checked_list{$jobid});
	print "></td>";

	# Print each column of the row
	foreach my $item (@list)
		{
		if($item ne '')
			{ print "<td nowrap>", html($item), "</td>" }
		else
			{ print "<td>&nbsp;</td>" }
		}

	print "</tr>\n";
	}

# Make some empty rows to suggest the possibility of more jobs.
for(; $jobcount < 5; $jobcount++)
	{
	print "<tr><th><input type=\"checkbox\" value=\"\"></th>";
	foreach my $i (@fields)
		{ print "<td>&nbsp;</td>" }
	print "</tr>\n";
	}

print <<"TFOOT10";
</tbody>
</table>
</div>
TFOOT10

#=============================================================
# This is the end of the exception handling block.
#=============================================================
}; if($@)
	{
	print "<p>$@</p>\n";
	}

#=============================================================
# Final stuff
#=============================================================

if($data{controls})
{
# Start the bottom menubar.
print "<div class=\"menubar\" style=\"$fixed_div_style_bottom\">\n";

# Create the move select box.
if(defined $controls{Move})
{
print <<"QuoteMove10";
<select name="move_to"
		onchange="document.forms[0].submit()"
		onmouseover="page_lock()"
		onmouseout="page_unlock()"
		>
<option value="" selected>Move To
QuoteMove10
foreach my $i (split(/ /, $data{dests}))
	{
	$i = html($i);
	print "<option value=", html_value($i), ">", html($i), "\n";
	}
print <<"QuoteMove90";
</select>
<noscript>
<input class="buttons" type="submit" name="action" value="Move">
</noscript>
QuoteMove90
}

# Create all of the other buttons.
isubmit("action", "Cancel", N_("_Cancel"), _("Cancel selected jobs.")) if(defined $controls{Cancel});
isubmit("action", "Rush", N_("R_ush"), _("Move selected jobs to the front of the queue.")) if(defined $controls{Rush});
isubmit("action", "Hold", N_("_Hold"), _("Place selected jobs in the held state so that they won't print.")) if(defined $controls{Hold});
isubmit("action", "Release", N_("_Release"), _("Release selected jobs which are held.")) if(defined $controls{Release});
isubmit("action", "Modify", N_("_Modify"), _("Open windows in which to modify selected jobs."), "return do_modify()") if(defined $controls{Modify});
isubmit("action", "Log", N_("_Log"), _("Display log files of selected jobs."), "return do_log()") if(defined $controls{Log});

# Close the bottom menubar.
print "</div>\n";
}

# Create those hidden fields and close the form.
cgi_write_data();
print "</form>\n";

# During testing, print data dump at bottom of page.
cgi_debug_data() if($DEBUG);

print <<"Quote10";
<script>
window.setTimeout("gentle_reload()", $refresh_interval * 1000);
</script>
</body>
</html>
Quote10

exit 0;

