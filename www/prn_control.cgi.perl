#! @PERL_PATH@ -wT
#
# mouse:~ppr/src/www/prn_control.cgi.perl
# Copyright 1995--2007, Trinity College Computing Center.
# Written by David Chappell.
#
# This file is part of PPR.  You can redistribute it and modify it under the
# terms of the revised BSD licence (without the advertising clause) as
# described in the accompanying file LICENSE.txt.
#
# Last modified 1 June 2007.
#

use lib "@PERL_LIBDIR@";
use PPR::PPOP;
require 'cgi_data.pl';
require 'cgi_time.pl';
require 'cgi_intl.pl';
require 'cgi_back.pl';
require 'cgi_widgets.pl';
require 'cgi_user_agent.pl';

# How wide is a character (in pixels)?
my $CHARWIDTH = 10;

# How wide (in pixels) should a progress bar be?
my $progress_width_total = 550;

# Swap the real and effective user ids.
($<,$>) = ($>,$<);

# Initialize the internationalization libraries and determine the
# content language and charset.
my ($charset, $content_language) = cgi_intl_init();

# Read the posted form data.
&cgi_read_data();

# Should we have controls?
my $controls = cgi_data_peek("controls", 1);

# Find the printer name.  We will put it into the form submit URL.
my $printer = cgi_data_move('name', '_missing_name_');

# Figure out which button the user pressed, if any.
my $action = cgi_data_move('action', undef);

# If the user press the "Close" button and it got past
# JavaScript, try to load the previous page.
if(defined $action && $action eq "Close")
	{
	cgi_back_doit();
	exit 0;
	}

# What back information should we pass to scripts to which we link?
my $encoded_back_stack = cgi_back_init();

# Demand authentication if necessary.
if(defined($action) && $action ne "Refresh" && undef_to_empty($ENV{REMOTE_USER}) eq "")
	{
	require "cgi_auth.pl";
	demand_authentication();
	exit 0;
	}

my $title = html(sprintf(_("Status of Printer \"%s\""), $printer));

print <<"EndOfHead";
Content-Type: text/html;charset=$charset
Content-Language: $content_language
Vary: accept-language

<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd">
<html>
<head>
<title>$title</title>
<meta http-equiv="Content-Style-Type" content="text/css">
<link rel="stylesheet" href="../style/shared.css" type="text/css">
<link rel="stylesheet" href="../style/prn_control.css" type="text/css">
<script type="text/javascript" src="../js/show_queues.js" defer></script>
</head>
<body>
EndOfHead

# This is the start of an exception handling block:
eval
{
# Connect to ppop.
my $control = new PPR::PPOP($printer);

# Perform any button actions.
my @action_result = ();
if(defined($action) && $action ne "Refresh")
	{
	# Set the --user option for ppop:
	$control->user("$ENV{REMOTE_USER}\@$ENV{REMOTE_ADDR}");

	# Call the correct member function to perform the action.
	if($action eq 'Start')
		{
		@action_result = $control->start();
		}
	elsif($action eq 'Stop')
		{
		@action_result = $control->stop();
		}
	elsif($action eq 'Halt')
		{
		@action_result = $control->halt();
		}
	else
		{
		die "Unrecognized action \"$action\".\n";
		}
	}

# If the action returned an error message, emmit code to
# print a javascript error window, or failing that,
# to print a line at the top of the window.
if((scalar @action_result) != 0)
	{
	require 'cgi_error.pl';
	error_window(sprintf(_("Can't [%s] printer \"%s\" for the following reason:"), $action, $printer), @action_result);
	}

# Fetch the description of the printer.
my $comment = $control->get_comment();

# Get the printer status.
my @result_rows = $control->get_pstatus();
my @result_row1 = @{shift @result_rows};

# Sanity check: make sure the first field is the printer name.
((shift @result_row1) eq $printer) || die "Failed to get status of printer \"$printer\"";

# The first three columns are easy.
my $status = shift @result_row1;
my $job = shift @result_row1;
my $retry = shift @result_row1;
my $countdown = shift @result_row1;

# The remaining columns are auxiliary status information.
my @printer_status = ();
my $printer_job = "";
my $operation = "";
my $page_clock = "";
foreach my $i (@result_row1)
	{
	if($i =~ /^status: (.+)$/)					# unified SNMP style
		{
		push(@printer_status, $1);
		}
	elsif($i =~ /^errorstate: (.+)$/)			# unified SNMP style, faults
		{
		push(@printer_status, $1);
		}
	elsif($i =~ /^.+-status: (\d+) (.+)$/)		# various raw
		{
		if($1)	# if important
			{
			push(@printer_status, $2);
			}
		}
	elsif($i =~ /^job: (.+)$/)					# name of current job
		{
		$printer_job = $1;
		}
	elsif($i =~ /^operation: (.+)$/)			# pprdrv operation
		{
		$operation = $1;
		}
	elsif($i =~ /^page: (.+)$/)					# page clock value
		{
		$page_clock = $1;
		}
	}
my $printer_status = join("\n", @printer_status);

# If we think it is worth it, get the alerts log.
my $alerts = "";
if($status ne "printing" && $status ne "idle")
   { $alerts = $control->get_alerts() }

# If a job is being printed, get the progress.
my($progress_job, $forwhom, $bytes_sent, $bytes_total, $percent_sent, $pages_started, $pages_completed, $pages);
if($job ne "")
	{
	($progress_job, $forwhom, $bytes_sent, $bytes_total, $percent_sent, $pages_started, $pages_completed, $pages) = $control->get_progress($job);
	}
else
	{
	($progress_job, $forwhom, $bytes_sent, $bytes_total, $percent_sent, $pages_started, $pages_completed, $pages) = ("", "", "", "", "", 0, 0, -1);
	}

# Shut down ppop.
$control->destroy();

# Convert the status to human readable form.
$status = humanify_printer_status($status, $job, $retry, $countdown, $printer_status);

# If we are printing a job, display that job name.	If not and the printer
# reports that it is printing a job for someone else, display that name
# instead.
my $display_job = $job ne "" ? $job : $printer_job;

# Encode everything except the numbers as HTML.
#$printer = html($printer);
#$comment = html($comment);
#$status = html($status);
$printer_status = html($printer_status);
$printer_status =~ s/\n/<br>/g;
#$operation = html($operation);
#$page_clock = html($page_clock);
#$forwhom = html($forwhom);
#$display_job = html($display_job);

# Prepare the display versions of some of the progress variables.  We
# suppress zero and negative one when they really mean "no information
# available".
my $html_pages = ($pages != -1) ? html($pages) : "";
my $html_pages_started = ($pages_started > 0 || $pages > 0) ? html($pages_started) : "";
my $html_pages_completed = ($pages_completed > 0) ? html($pages_completed) : "";

my $user_agent = cgi_user_agent();
my $border = $user_agent->{css} ? 0 : 1;

# Start a form and within it a 2 cell table.  The left hand cell will
# present information, the right hand one will contain the buttons.
print <<"Form10";
<form action="$ENV{SCRIPT_NAME}?${\&form_urlencoded("name", $printer)}" method="post">
<table width="100%" height="100%">
<tr>
<td align="left" valign="top">
Form10

print "<table border=$border><tr>\n";
print "<th>", H_NB_("Name:"), "</th><td width=\"", $CHARWIDTH * 10, "\">", html($printer), "</td>\n";
print "<th>", H_NB_("Comment:"), "</th><td width=\"", $CHARWIDTH * 32, "\">", html_nb($comment), "</td>\n";
print "</tr></table>\n";

print "<table border=$border><tr>\n";
print "<th>", H_NB_("Status:"), "</th><td width=\"", $CHARWIDTH * 28, "\">", html_nb($status), "</td>\n";
print "<th>", H_NB_("Retry:"), "</th><td width=\"", $CHARWIDTH * 4, "\">$retry</td>\n";
print "<th>", H_NB_("Countdown:"), "</th><td width=\"", $CHARWIDTH * 4, "\">$countdown</td>\n";
print "</tr></table>\n";

print "<table border=$border><tr>\n";
print "<th>", H_NB_("Operation:"), "</th><td width=\"", $CHARWIDTH * 25, "\">", html_nb($operation), "&nbsp;</td>\n";
print "<th>", H_NB_("Page clock:"), "</th><td width=\"", $CHARWIDTH * 15, "\">", html_nb($page_clock), "&nbsp;</td>\n";
print "</tr></table>\n";

print "<table border=$border><tr>\n";
print "<th>", H_NB_("Printer status:"), "</th><td width=\"", $CHARWIDTH * 50, "\">$printer_status&nbsp;</td>\n";
print "</tr></table>\n";

print "<table border=$border><tr>\n";
print "<th>", H_NB_("Current job:"), "</th><td width=\"", $CHARWIDTH * 20, "\">", html_nb($display_job), "&nbsp;</td>\n";
print "<th>", H_NB_("For:"), "</th><td width=\"", $CHARWIDTH * 25, "\">", html_nb($forwhom), "&nbsp;</td>\n";
print "</tr></table>\n";

print "<table border=$border><tr>\n";
print "<th>", H_NB_("Total bytes:"), "</th><td width=\"", $CHARWIDTH * 9, "\">${\bytes($bytes_total)}&nbsp;</td>\n";
print "<th>", H_NB_("Bytes sent:"), "</th><td width=\"", $CHARWIDTH * 9, "\">${\bytes($bytes_sent)}&nbsp;</td>\n";
print "<th>", H_NB_("Percent sent:"), "</th><td width=\"", $CHARWIDTH * 4, "\">$percent_sent&nbsp;</td>\n";
print "</tr></table>\n";

print "<table border=$border><tr>\n";
print "<th>", H_NB_("Total pages:"), "</th><td width=\"", $CHARWIDTH * 4, "\">$html_pages&nbsp;</td>\n";
print "<th>", H_NB_("Pages started:"), "</th><td width=\"", $CHARWIDTH * 4, "\">$html_pages_started&nbsp;</td>\n";
print "<th>", H_NB_("Pages completed:"), "</th><td width=\"", $CHARWIDTH * 4, "\">$html_pages_completed&nbsp;</td>\n";
print "</tr></table>\n";

# If it is printing a job, display the progress bar.
if($job ne "")
	{
	# The red part is proportional to the percentage sent.
	my $width1 = int($percent_sent * $progress_width_total / 100);

	# Clip the red part at 0% and 100%.
	if($width1 < 0)
		{ $width1 = 0 }
	elsif($width1 > $progress_width_total)
		{ $width1 = $progress_width_total }

	# The blue part is whatever is left over.
	my $width2 = ($progress_width_total - $width1);

	print "<span class=\"label\"><span class=\"sent\">", H_("Bytes Sent"), "</span>/<span class=\"unsent\">", H_("Bytes Unsent"), "</span></span><br>\n";
	print "<img class=\"left\" src=\"../images/pixel-red.png\" hspace=0 height=25 width=$width1>";
	print "<img class=\"right\" src=\"../images/pixel-blue.png\" hspace=0 height=25 width=$width2>";
	print "<br>\n";
	}

# If we know how many pages there should be, and the values make sense,
# display a 3 value progress bar.
print "<!-- pages_completed: $pages_completed, pages_started: $pages_started, total_pages: $pages -->\n";
if($pages > 0 && $pages_started <= $pages && $pages_completed <= $pages && $pages_completed <= $pages_started)
	{
	# The red bar is proportional to the pages completed (probably
	# as reported by HP PJL).
	my $width1 = int($pages_completed * $progress_width_total / $pages);

	# The bar of pages started is proportional to the number of pages
	# started, but the pixels for the 1st (red) bar are subtracted since
	# they `overlap' for the length of the red bar.
	my $width2 = int($pages_started * $progress_width_total / $pages);
	$width2 -= $width1;

	# The third bar is what is left over.
	my $width3 = ($progress_width_total - $width1 - $width2);

	# No bar should be less than 0 pixels long or longer than the total.
	print "<!-- Widths: $width1, $width2, $width3 -->\n";
	die if($width1 < 0 || $width1 > $progress_width_total);
	die if($width2 < 0 || $width2 > $progress_width_total);
	die if($width3 < 0 || $width3 > $progress_width_total);

	print "<span class=\"label\">\n";
		print "<span class=\"completed\">", H_NB_("Pages Completed"), "</span>/" if($width1 > 0);
		print "<span class=\"started\">", H_NB_("Pages Started"), "</span>/";
		print "<span class=\"unstarted\">", H_NB_("Pages Unstarted"), "</span>";
	print "</span>\n";
	print "<br>\n";
		print "<img class=\"left\" src=\"../images/pixel-white.png\" hspace=0 height=25 width=$width1>"; # no "\n"!
		print "<img class=\"middle\" src=\"../images/pixel-red.png\" hspace=0 height=25 width=$width2>"; # no "\n"!
		print "<img class=\"right\" src=\"../images/pixel-blue.png\" hspace=0 height=25 width=$width3>"; # no "\n"!
	print "<br>\n";
	}

#
# If we retrieved an alerts log, print it.	Notice that again,
# this form entry widget has no name.
#
if($alerts ne "" && $controls)
{
print "<label><span class=\"label\">", H_("Alert Messages:"), "</span><br>\n";
print <<"Form18";
<textarea class="alerts" rows=8 cols=80 wrap="off" tabindex=0 readonly>
${\html($alerts)}
</textarea>
</label>
Form18
}

print "</td>\n";

# "Draw" all of the control buttons.
if($controls)
	{
	print "<td class=\"right_buttons\" align=\"right\" valign=\"top\">\n";
	isubmit("action", "Start", N_("_Start"), _("Start the printer."));
	print "<br>\n";
	isubmit("action", "Stop", N_("Sto_p"), _("Stop the printer after current job."));
	print "<br>\n";
	isubmit("action", "Halt", N_("_Halt"), _("Stop the printer imediately."));
	print "<br>\n";
	link_button(_("Media"), "prn_media.cgi?" . form_urlencoded("name", $printer) . ";" . $encoded_back_stack,
		_("Change the media mounted on the printer's input trays."));
	print "<br><br><br>\n";
	isubmit("action", "Refresh", N_("_Refresh"));
	print "<br>\n";
	# We use parent.close() in the next line so that if it opened in a frame by
	# grp_control() it will close the window rather than trying to close the
	# frame (which can't be done).  If the window is not in a frame, it is its
	# own parent, so this still works.
	isubmit("action", "Close", N_("_Close"), _("Close this window."), "parent.close()");
	print "</td>\n";
	}

print "</tr></table>\n";

# If the printer status has changed, call for the reloading the queue list
# window (the window that opened this window) so that it will display an icon 
# which represents the current status of this printer.
if(defined($data{prev_status}))
	{
	if($status ne $data{prev_status})
		{
		print "<script>window.opener.gentle_reload()</script>\n";
		}
	}
$data{prev_status} = $status;

# Snap the window size to fit snugly around the document.  This is disabled
# because some forms of this window have a lot of blank space at the bottom.
if(0 && !cgi_data_peek("resized", 0))
{
print <<"Tail05";
<script>
if(document.width)
	{
	window.resizeTo(document.width, document.height);
	}
</script>
Tail05
$data{resized} = 1;
}

# Propagate data to the next generation.
&cgi_write_data();

print "</form>\n";

# This is the end of the exception handling block.	If die() was called
# within the block, print its message.
}; if($@)
		{
		my $message = html($@);
		print "<p>$message</p>\n";
		}

# Use Javascript to arrange for this page to be refreshed.	Notice that for
# pre-5.0 Netscape we don't use submit() because Netscape submits the value
# of the first button!
print <<"Tail01";
<script>
var browser_version = parseFloat(navigator.appVersion);
if(browser_version < 5.0 && navigator.appName.indexOf("Microsoft") == -1)
		{ window.setTimeout("document.location.reload()", 10000); }
else
		{ window.setTimeout("document.forms[0].submit()", 10000); }
</script>
Tail01

print <<"Tail10";
</body>
</html>
Tail10

exit 0;

=pod

This may be moved to a library files later.	 It can change the rather terse
machine-readable queue status strings into something a little clearer.	It
is also where we translate the messages into languages other than English.

=cut
sub humanify_printer_status
	{
	my($status, $job, $retry, $countdown, $message) = @_;

	if($status eq "fault")
		{
		if($retry > 0)
			{ $status = _("fault") }
		else
			{ $status = _("fault, no auto retry") }
		}

	elsif($status eq "engaged")
		{
		if($message =~ /off\s?line/i)
			{ $status = _("offline") }
		else
			{ $status = _("otherwise engaged or offline") }
		}

	elsif($status eq "idle")
		{
		$status = _("idle");
		}

	elsif($status eq "stopt")
		{
		$status = _("stopt");
		}

	return $status;
	}

#
# print a number of bytes, using kilobytes and megabytes for sufficiently 
# large values.
#
sub bytes
	{
	my $count = shift;
	return $count if($count < 8192);
	return sprintf("%.1fK", $count / 1024) if($count < 1_048_576);
	return sprintf("%.1fM", $count / 1_048_576);
	}

# end of file

