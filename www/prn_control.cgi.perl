#! /usr/bin/perl -wT
#
# mouse:~ppr/src/www/prn_control.cgi.perl
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
# Last modified 24 February 2002.
#

use lib "?";
use PPR::PPOP;
require 'cgi_data.pl';
require 'cgi_time.pl';
require 'cgi_intl.pl';

# How wide is a character (in pixels)?
my $CHARWIDTH = 10;

# How wide (in pixels) should a progress bar be?
my $progress_width_total = 550;

# This may be moved to a library files later.  It can change the rather terse
# machine-readable status strings into something a little clearer.  It is also
# where we translate the messages into languages other than English.
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
if(defined($action))
    {
    if($action eq "Close")
	{
	require 'cgi_back.pl';
	cgi_back_doit();
	exit 0;
	}
    if($action eq "Media")
	{
	require 'cgi_redirect.pl';
	my $encoded_HIST = form_urlencoded("HIST", $data{HIST});
	my $encoded_name = form_urlencoded("name", $printer);
        cgi_redirect("http://$ENV{SERVER_NAME}:$ENV{SERVER_PORT}/cgi-bin/prn_media.cgi?$encoded_name;$encoded_HIST");
        exit 0;
	}
    }

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
<link rel="stylesheet" href="../style/prn_control.css" type="text/css">
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
    # Set the user id for ppop:
    die if(undef_to_empty($ENV{REMOTE_USER}) eq "");
    $control->su($ENV{REMOTE_USER});

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
((shift @result_row1) eq $printer) || die "Failed to get printer status";

# The first three columns are easy.
my $status = shift @result_row1;
my $job = shift @result_row1;
my $retry = shift @result_row1;
my $countdown = shift @result_row1;

# The remaining ones are auxiliary status information.
my $printer_lw_status = "";
my $printer_status_job = "";
my $operation = "";
my $page_clock = "";
foreach my $i (@result_row1)
    {
    if($i =~ /^lw-status: (.+)$/ || $i =~ /^pjl: (.+)$/)
    	{
    	$printer_lw_status = $1;
    	}
    elsif($i =~ /^job: (.+)$/)
    	{
	$printer_status_job = $1;
    	}
    elsif($i =~ /^operation: (.+)$/)
    	{
    	$operation = $1;
    	}
    elsif($i =~ /^page: (.+)$/)
    	{
    	$page_clock = $1;
    	}
    }

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
$status = humanify_printer_status($status, $job, $retry, $countdown, $printer_lw_status);

# If we are printing a job, display that job name.  If not and the printer
# reports that it is printing a job for someone else, display that name
# instead.
my $display_job = $job ne "" ? $job : $printer_status_job;

# Encode everything except the numbers as HTML.
$printer = html($printer);
$comment = html($comment);
$status = html($status);
$printer_lw_status = html($printer_lw_status);
$operation = html($operation);
$page_clock = html($page_clock);
$forwhom = html($forwhom);
$display_job = html($display_job);

# Prepare the display versions of some of the progress variables.  We
# suppress zero and negative one when they really mean "no information
# available".
my $html_pages = ($pages != -1) ? html($pages) : "";
my $html_pages_started = ($pages_started > 0 || $pages > 0) ? html($pages_started) : "";
my $html_pages_completed = ($pages_completed > 0) ? html($pages_completed) : "";

# Start a form and within it a 2 cell table.  The left hand cell will
# present information, the right hand one will contain the buttons.
print <<"Form10";
<form action="$ENV{SCRIPT_NAME}?${\&form_urlencoded("name",$printer)}" method="post">
<table width="100%" height="100%">
<tr>
<td align="left" valign="top">
Form10

if(0)
{
# Do it with forms.  This is a little annoying because it offers the user an
# opportunity to enter information, when in fact doing so is useless.
# We do our best to fix this by including the tabindex=0 and readonly
# attributes of HTML 4.0.
#
# Notice that the entry widgets have no names so that they will not waste
# bandwidth sending stuff to the server.
print <<"Form12";
	<span class="label">${\H_("Name:")}</span> <span class="blank">$printer<spacer type="horizontal" size=50></span>
	<span class="label">${\H_("Comment:")}</span> <span class="blank">$comment</span>
<br>
	<label><span class="label">${\H_("Status:")}</span> <input type="text" size=28 value="$status" tabindex=0 readonly></label>
	<label><span class="label">${\H_("Retry:")}</span> <input type="text" size=4 value="$retry" tabindex=0 readonly></label>
	<label><span class="label">${\H_("Countdown:")}</span> <input type="text" size=4 value="$countdown" tabindex=0 readonly></label>
<br>
	<label><span class="label">${\H_("Operation:")}</span> <input type="text" size=25 value="$operation" tabindex=0 readonly></label>
	<label><span class="label">${\H_("Page clock:")}</span> <input type="text" size=20 value="$page_clock" tabindex=0 readonly></label>
<br>
	<label><span class="label">${\H_("Printer status:")}</span> <input type="text" size=50 value="$printer_lw_status" tabindex=0 readonly></label>
<br>
	<label><span class="label">${\H_("Current job:")}</span> <input type="text" size=25 value="$display_job" tabindex=0 readonly></label>
	<label><span class="label">${\H_("For:")}</span> <input type="text" size=25 value="$forwhom" tabindex=0 readonly></label>
<br>
	<label><span class="label">${\H_("Total bytes:")}</span> <input type="text" size=9 value="$bytes_total" tabindex=0 readonly></label>
	<label><span class="label">${\H_("Bytes sent:")}</span> <input type="text" size=9 value="$bytes_sent" tabindex=0 readonly></label>
	<label><span class="label">${\H_("Percent sent:")}</span> <input type="text" size=4 value="$percent_sent" tabindex=0 readonly></label>
<br>
	<lable><span class="label">${\H_("Total pages:")}</span> <input type="text" size=4 value="$html_pages" tabindex=0 readonly></label>
	<label><span class="label">${\H_("Pages started:")}</span> <input type="text" size=4 value="$html_pages_started" tabindex=0 readonly></label>
	<label><span class="label">${\H_("Pages completed:")}</span> <input type="text" size=4 value="$html_pages_completed" tabindex=0 readonly></label>
<br>
Form12
}

else
{
# This is the other way.  We try to do it with nested tables.
print <<"Form13";
<table><tr>
	<th>${\H_("Name:")}</th><td width="${\($CHARWIDTH * 10)}">$printer</td>
	<th>${\H_("Comment:")}</th><td width="${\($CHARWIDTH * 32)}">$comment</td>
</tr></table>

<table><tr>
	<th>${\H_("Status:")}</th><td width="${\($CHARWIDTH * 28)}">$status</td>
	<th>${\H_("Retry:")}</th><td width="${\($CHARWIDTH * 4)}">$retry</td>
	<th>${\H_("Countdown:")}</th><td width="${\($CHARWIDTH * 4)}">$countdown</td>
</tr></table>

<table><tr>
	<th>${\H_("Operation:")}</th><td width="${\($CHARWIDTH * 25)}">$operation&nbsp;</td>
	<th>${\H_("Page clock:")}</th><td width="${\($CHARWIDTH * 15)}">$page_clock&nbsp;</td>
</tr></table>

<table><tr>
	<th>${\H_("Printer status:")}</th><td width="${\($CHARWIDTH * 40)}">$printer_lw_status&nbsp;</td>
</tr></table>

<table><tr>
	<th>${\H_("Current job:")}</th><td width="${\($CHARWIDTH * 20)}">$display_job&nbsp;</td>
	<th>${\H_("For:")}</th><td width="${\($CHARWIDTH * 25)}">$forwhom&nbsp;</td>
</tr></table>

<table><tr>
	<th>${\H_("Total bytes:")}</th><td width="${\($CHARWIDTH * 9)}">$bytes_total&nbsp;</td>
	<th>${\H_("Bytes sent:")}</th><td width="${\($CHARWIDTH * 9)}">$bytes_sent&nbsp;</td>
	<th>${\H_("Percent sent:")}</th><td width="${\($CHARWIDTH * 4)}">$percent_sent&nbsp;</td>
</tr></table>

<table><tr>
	<th>${\H_("Total pages:")}</th><td width="${\($CHARWIDTH * 4)}">$html_pages&nbsp;</td>
        <th>${\H_("Pages started:")}</th><td width="${\($CHARWIDTH * 4)}">$html_pages_started&nbsp;</td>
	<th>${\H_("Pages completed:")}</th><td width="${\($CHARWIDTH * 4)}">$html_pages_completed&nbsp;</td>
</tr></table>
Form13
}

# If it is printing, display the progress bar.
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
        print "<span class=\"completed\">", H_("Pages Completed"), "</span>/" if($width1 > 0);
    	print "<span class=\"started\">", H_("Pages Started"), "</span>/";
    	print "<span class=\"unstarted\">", H_("Pages Unstarted"), "</span>";
    print "</span><br>\n";
	print "<img class=\"left\" src=\"../images/pixel-white.png\" hspace=0 height=25 width=$width1>"; # no "\n"!
	print "<img class=\"middle\" src=\"../images/pixel-red.png\" hspace=0 height=25 width=$width2>"; # no "\n"!
	print "<img class=\"right\" src=\"../images/pixel-blue.png\" hspace=0 height=25 width=$width3>"; # no "\n"!
    print "<br>\n";
    }

#
# If we retrieved an alerts log, print it.  Notice that again,
# this form entry widget has no name.
#
if($alerts ne "" && $controls)
{
print <<"Form18";
<label><span class="label">${\H_("Alert Messages:")}</span><br>
<textarea class="alerts" rows=6 cols=70 wrap="off" tabindex=0 readonly>
$alerts
</textarea>
</label>
Form18
}

print "</td>\n";

# "Draw" all of the control buttons.
if($controls)
{
print "<td align=\"right\" valign=\"top\">\n";
isubmit("action", "Start", N_("_Start"), "class=\"buttons\"");
print "<br>\n";
isubmit("action", "Stop", N_("Sto_p"), "class=\"buttons\"");
print "<br>\n";
isubmit("action", "Halt", N_("_Halt"), "class=\"buttons\"");
print "<br>\n";
isubmit("action", "Media", N_("_Media"),
	"class=\"buttons\" onclick=\"window.open('prn_media.cgi?name=" . html($printer) . "', '_blank', 'width=700,height=300,scrollbars,resizable'); return false;\""
	);
print "<br><br><br>\n";
isubmit("action", "Refresh", N_("_Refresh"), "class=\"buttons\"");
print "<br>\n";
isubmit("action", "Close", N_("_Close"), "class=\"buttons\" onclick=\"window.close()\"");
print "</td>\n";
}

print "</tr></table>\n";

# If the printer status has changed, call for the reloading the queue list
# window (the parent of this window) so that it will display an icon which
# represents the current status of this printer.
if(defined($data{prev_status}))
    {
    if($status ne $data{prev_status})
    	{
	print "<script>window.opener.gentle_reload()</script>\n";
    	}
    }
$data{prev_status} = $status;

# Propagate data to the next generation.
&cgi_write_data();

print <<"Form50";
</form>
Form50

# This is the end of the exception handling block.  If die() was called
# within the block, print its message.
}; if($@)
	{
	my $message = html($@);
	print "<p>$message</p>\n";
	}

# Use Javascript to arrange for this page to be refreshed and then close the
# document.  Notice that for pre-5.0 Netscape we don't use submit() because
# Netscape submits the value of the first button!
print <<"Tail";
<script>
var browser_version = parseFloat(navigator.appVersion);
if(browser_version < 5.0 && navigator.appName.indexOf("Microsoft") == -1)
	{ window.setTimeout("document.location.reload()", 10000); }
else
	{ window.setTimeout("document.forms[0].submit()", 10000); }
</script>
</body>
</html>
Tail

exit 0;

# end of file

