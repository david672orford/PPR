#! /usr/bin/perl -wT
#
# mouse:~ppr/src/www/show_printlog.cgi.perl
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
# Last modified 15 August 2002.
#

use lib "?";
require 'paths.ph';
require 'cgi_data.pl';
require 'cgi_intl.pl';
require 'cgi_time.pl';
defined($LOGDIR) || die;
defined($USER_PPR) || die;

# How far into the future (in seconds) should we declare the expiration date
# to be?
$LIFETIME = 60;

# This is the file we will be formatting and displaying.
$filename = "$LOGDIR/printlog";

# Initialize the internationalization libraries and determine the
# content language and charset.
my ($charset, $content_language) = cgi_intl_init();

# Read the CGI form variables.
&cgi_read_data();

# Jobs which originated on and were printed on this node get
# shortened ids.
my $blank_node = (split(/\./, $ENV{SERVER_NAME}))[0];

my $queue = cgi_data_move('queue', undef);
my $printer = cgi_data_move('printer', undef);

# Adjust the title according to what we are filtering for.
if(defined($queue))
    { $title = html(sprintf(_("PPR Print Log for Queue \"%s\""), $queue)) }
elsif(defined($printer))
    { $title = html(sprintf(_("PPR Print Log for Printer \"%s\""), $printer)) }
else
    { $title = H_("PPR Print Log") }

# Make the last modification time of the file be the HTML file be the same
# as that of the log file it was created from.
{
my($mtime) = (stat($filename))[9];
if(defined($mtime))
    {
    print "Last-Modified: ", cgi_time_format($mtime), "\n";
    print "Expires: ", cgi_time_format(time() + $LIFETIME), "\n";
    }
}

print <<"LogHead";
Content-Type: text/html;charset=$charset
Content-Language: $content_language
Vary: accept-language

<html>
<head>
<title>$title</title>
<style type="text/css">
BODY { background: white; color: black; }
TH, TD { text-align: left; padding: 0.5mm 1.0mm; }
</style>
</head>
<body>
<h1>$title</h1>
LogHead

# Try:
eval {

if(!open(LOG, "< $filename"))
    {
    my $error = $!;
    if($error =~ /^No such file /)
    	{
    	die sprintf(_("Print job logging is disabled.  To enable it, create the file \"%s\"\n"
    		. "and make sure the user \"%s\" can write to it.\n"), $filename, $USER_PPR);
    	}
    else
	{
	die sprintf(_("Can't open log file \"%s\", %s\n"), $filename, $!);
	}
    }

my $last_qdate = '';
my @row;
my $qdate;
while(<LOG>)
    {
    #    date, jobid,  printer,for,      user,    proxyfor,pages,    shts,     sds,      qtime,    prnttime,charge,        pjlpages,    scount,      increment    psbytes,    totalbytes
    #    1     2       3       4         5        6        7         8         9         10        11       12-13          14-15        16-17        18-19        20-21       22-23
    if(/^(\d+),([^,]+),([^,]+),"([^"]+)",([^,]+),"([^"]*)",([-0-9]+),([-0-9]+),([-0-9]+),([\d\.]+),([\d\.]+)(,([-0-9\.]+))?(,([-0-9]+))?(,([-0-9]+))?(,([-0-9]+))?(,([0-9]+))?(,([0-9]+))?(,"([^"]*)")?$/)
        {
        @row = ($1,$2,$3,$4,$5,$6,$7,$8,$9,$10,$11,$13,$15,$17,$19,$21,$23,$25);
        my $datetime = shift @row;
        $datetime =~ /^(\d{4})(\d{2})(\d{2})(\d{2})(\d{2})(\d{2})$/;
        $qdate = "$1-$2-$3";
        unshift(@row, "$4:$5:$6");
        }
    else
        {
        $qdate = "?";
        @row = qw(? ? ?);
        }

    # If we are filtering on queue name and the job name is parsable and
    # it doesn't match, skip it.
    if(defined($queue))
        {
        if($row[1] =~ /^[^:]+:(.+)-\d+\.\d+\(.+\)$/)
            {
            next if($1 ne $queue);
            }
        }

    # If filtering by printer and this is not the right one, skip it.
    next if(defined($printer) && $row[2] ne $printer);

    # If the job both originated and was printed on this
    # system, then shorten the job id.
    if($row[1] =~ /^([^:]+):([^\(]+)\(([^\)]+)\)$/)
        {
        if($1 eq $3 && $1 eq $blank_node)
            {
            $row[1] = $2;
            $row[1] =~ s/\.0$//;
            }
        }

    # If the date has changed, start a new table.
    if($qdate ne $last_qdate)
        {
	# If there was a previous table, close it.
        if($last_qdate ne "")
            {
            print "</table>\n\n";
            }

        print "<h2>", html($qdate), "</h2>\n";
        print "<table border=1 cellspacing=0>\n";
        print "<tr>";
	foreach my $field (N_("Time"), N_("Job ID"), N_("Printer"), N_("For"), N_("Unix User"), N_("Proxy For"),
			N_("Pages"), N_("Total Sheets"), N_("Total Sides"), N_("Queued Time"), N_("Printing Time"),
			N_("Charge"), N_("PJL Pages"), N_("Start Pages"), N_("Increment"), N_("PS Bytes"), N_("Bytes Sent"),
			N_("Title or Filename"))
		{
		print "<th scope=\"col\">", H_($field), "</th>";
		}
	print "</tr>\n";

        $last_qdate = $qdate;
        }

    # Emmit one row
    print "<tr>";
    my $index = 0;
    foreach my $data (@row)
        {
	$index++;

	# The job id is a header field.
	if($index == 2)
	    {
	    print "<th nowrap scope=\"row\">", html($data), "</th>";
	    }

	# All others are just plain data.
	else
	    {
	    # Fields with spaces should not be wrapped.
	    if($data =~ /\s/)
		{ print "<td nowrap>" }
	    # Numberic fields should be right aligned.
	    elsif($data =~ /^([-0-9\.]+)$/)
		{ print "<td align=\"right\">" }
	    # Others are up to the browser.
	    else
		{ print "<td>" }

	    if($data ne "")
		{ print html($data) }
	    else
	    	{ print "&nbsp;" }

	    print "</td>";
	    }
        }
    print "</tr>\n";
    }

close(LOG);

if($last_qdate ne "")
    {
    print "</table>\n";
    }

# Catch:
}; if($@)
    {
    my $message = html($@);
    print "<p>$message</p>\n";
    }

print <<"LogTail";
</body>
</html>
LogTail

exit 0;

