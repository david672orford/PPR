#! /usr/bin/perl -wT
#
# mouse:~ppr/src/www/df_html.cgi.perl
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
# Last modified 9 March 2004.
#

#
# This script runs the df command to get the disk spaces statistics for all
# of the local file systems.  It then generates a series of little tables
# which hold captions and inline images of pie charts.	The inline images
# are generated later by another CGI script called df_img.cgi.
#

use lib "?";
require 'paths.ph';
require 'cgi_time.pl';
require 'cgi_data.pl';
require 'cgi_intl.pl';
use Sys::Hostname;

# This is because Sys::Hostname:hostname() might have to exec uname
# and because we have to run df.
defined($SAFE_PATH) || die;
$ENV{PATH} = $SAFE_PATH;

# Which language?
my ($charset, $content_language) = cgi_intl_init();

# Unset LANG for fear df will change its output.
delete $ENV{LANG};

# We consider these results good for 5 minutes.	 If people
# want better than that, they can reload.
my $expires_time = cgi_time_format((time() + 300));

# What is the name of this server?
my $servername = hostname();

# Internationalized title
my $title = html(sprintf(_("Disk Space Usage on %s"), $servername));

# Emmit the CGI header and the document title.
print <<"HEAD10";
Expires: $expires_time
Content-Type: text/html;charset=$charset
Content-Language: $content_language
Vary: accept-language

<html>
<head>
<title>$title</title>
</head>
<body bgcolor="white">
<h1>$title</h1>
HEAD10
print "<p>As of ", html(scalar localtime(time())), ".</p>\n";

# Begining of exception handling block.
eval {

# Run df and generate a table holding an inline image of a pie chart and
# a caption for each file system listed in the df output.
open(D, "df -kl |") || die sprintf(_("Failed to run df (%s)"), $!);
my $junk = <D>;			# header line
my $save = "";
while(<D>)
	{
	if(/^\S+$/)
		{
		$save = $_;
		next;
		}
	elsif(/^\s+/)
		{
		$_ = $save . $_;
		}
	$save = "";

	my($dev, $total, $used, $available, $percent, $mountpt) = split(/\s+/, $_);

	# Some pseudo file systems have a total size of 0.	We aren't interested
	# in them.
	next if($total <= 0);

	# Some things aren't real devices.	Skip them.
	next if($dev !~ m#^/#);

	# Deduce the amount of space reserved for use only be root.
	my $reserved = ($total - $used - $available);

	# Convert the amounts in kilobytes to percentages of the total
	# disk space.
	my $percent_used = int($used / $total * 100.0 + 0.5);
	my $percent_available = int($available / $total * 100.0 + 0.5);
	my $percent_reserved = int($reserved / $total * 100.0 + 0.5);

	# Create a version of the total disk size in kilobytes which has
	# commas every 3 decimal places.
	my $total_with_commas = $total;
	$total_with_commas =~ s/(\d)(\d\d\d)$/$1,$2/;
	while($total_with_commas =~ s/(\d)(\d\d\d,)/$1,$2/) { }

	print <<"BODY10";
<table border=1 hspace=5 vspace=5 cellpadding=5 cellspacing=0 align="left">
<tr><td><table border=0 cellspacing=0 cellpadding=0>
				${\percent_bar($percent_reserved, "blue")}
				${\percent_bar($percent_available, "green")}
				${\percent_bar($percent_used, "red")}
				</table>
		</td>
	<td>
		<img width=10 height=10 src="../images/pixel-blue.png"> ${percent_reserved}% Reserved
		<br>
		<img width=10 height=10 src="../images/pixel-green.png"> ${percent_available}% Available
		<br>
		<img width=10 height=10 src="../images/pixel-red.png"> ${percent_used}% Used
		</td>
	</tr>
<tr><td align="center" colspan=2>Partition $mountpt<br>Device $dev<br>${total_with_commas}K</td></tr>
</table>
BODY10

	}

# Close the pipe from df.
close(D) || die sprintf(_("Error while closing pipe from df (%s)"), $!);

# End of exception handling block.	If die was called,
# print the message.
}; if($@)
	{
	my $message = html($@);
	print "<br clear=\"left\">$message\n";
	}

# Close the HTML.
print <<"TAIL10";
</body>
</html>
TAIL10

# This is done in a function because browsers don't always render a zero 
# height image with zero pixels.
sub percent_bar
	{
	my $percent = shift;
	my $color = shift;
	if($percent > 0)
		{
		return <<"EndBar";
<tr><td><img width=100 height=$percent border=0 src="../images/pixel-$color.png"></td></tr>
EndBar
		}
	}

exit 0;
