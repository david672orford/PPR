#! /usr/bin/perl
#
# mouse:~ppr/src/pprd/pprd-question.perl
# Copyright 1995--2002, Trinity College Computing Center.
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
# Last modified 12 March 2002.
#

#
# This program will send a response to the Tcl/Tk script "pprpopup".
#

require 5.000;
use lib "?";
require 'paths.ph';
require 'pprpopup.pl';
require 'cgi_data.pl';
use Sys::Hostname;

# Set a maximum time this script can run.
alarm(30);

# We will catch all errors so as to customize the die message.
eval {

# This is because Sys::Hostname:hostname() might have to exec uname.
defined($SAFE_PATH) || die;
$ENV{PATH} = $SAFE_PATH;

# This isn't necessarily so.
my $host_and_port = hostname() . ":15010";

# Split the arguments out into individually named variables.
my($jobname, $qfile) = @ARGV;

# These are the variables that we will read form the queue file.
my $question;
my $response_responder;
my $response_address;
my $response_options;
my $magic_cookie;
my $title = "";

# Read the above values from the queue file.
open(QF, $qfile) || die "Can't open \"$qfile\", $!";
while(<QF>)
    {
    if(/^Question: (.+)$/)
    	{
    	$question = $1;
    	}
    elsif(/^Response: (\S+) (\S+) (.+)?$/)
    	{
    	($response_responder, $response_address, $response_options) = ($1, $2, $3);
    	}
    elsif(/^MagicCookie: (.+)$/)
    	{
    	$magic_cookie = $1;
    	}
    elsif(/^Title: (.+)$/)
    	{
    	$title = $1;
    	}
    }
close(QF) || die $!;

# Make sure we got everthing that is required.
defined($question) || die "No question";
defined($response_address) || die "No response address";
defined($magic_cookie) || die "No magic cookie";

# Construct the query string.
my $query = join(';',
	form_urlencoded("jobname", $jobname),
	form_urlencoded("magic_cookie", $magic_cookie),
	form_urlencoded("title", $title)
	);
print STDERR "\$query=\"$query\"\n";

# Open a connexion to pprpopup
if(!open_connexion(SEND, $response_address))
    {
    die "open_conexion() failed\n";
    }

# Buffering would cause a lockup, so turn it off.
SEND->autoflush(1);

# Add the job to the job queue.
print SEND "JOB STATUS $jobname title=\"$title\"\n";
$result = <SEND>;

# Send the message to open the web page.
print SEND "QUESTION $jobname http://$host_and_port/$question?$query 6i 2i\n";
$result = <SEND>;

# Close the connexion to pprpopup.
close(SEND);

if($result !~ /^\+OK/)
    {
    die $result;
    }

# This is where we catch errors.
};
if($@)
    {
    print "pprd-question ", join(" ", @ARGV), ": $@";
    exit 1;
    }

exit 0;

# end of file
