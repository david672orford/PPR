#! /usr/bin/perl
#
# mouse:~ppr/src/responders/pprpopup.perl
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
# Last modified 20 November 2002.
#

#
# This program will send a response to the Tcl/Tk script "pprpopup"
# which is likely running on a MS-Windows 95 machine.
#

require 5.000;
use lib "?";
require 'respond.ph';
require 'pprpopup.pl';

# Set a maximum time this script can run.
alarm(30);

# Split the arguments out into individually named variables.
my($for, $addr, $msg, $msg2, $options, $code, $jobid, $extra, $title, $time, $reason, $pages, $charge) = @ARGV;

# Uncomment this code if you don't understand responder parameters and
# want to get a chance to see what they look like.	The output will
# appear on ppr's stdout or in the pprd log file.
if(0)
{
print "\$for = \"$for\"\n";
print "\$addr = \"$addr\"\n";
print "\$msg = \"$msg\"\n";
print "\$code = $code\n";
print "\$jobid = \"$jobid\"\n";
print "\$extra = \"$extra\"\n";
print "\$title = \"$title\"\n";
print "\$time = $time\n";
print "\$reason = \"$reason\"\n";
print "\$pages = $pages\n";
}

# Undo the cryptic arrest reason encoding.
$reason =~ s/,/, /g;
$reason =~ s/[|]/ /g;

# If a user is specified in the address, note the user part for future
# reference.
my $user = "";
if($addr =~ /^([^\@]+)\@.+$/)
	{
	$user = $1;
	}

# Open a TCP connexion to pprpopup on the user's machine.
if(!open_connexion(SEND, $addr))
  { exit(1) }

# Variable for response messages:
my $result;

# Remove the job from the list.
if(1)
	{
	my $jobname;
	($jobname = $jobid) =~ s/^(\S+) (\S+) (\d+) (\d+) (\S+)$/$1:$2-$3.$4($5)/;
	print SEND "JOB REMOVE $jobname\n";
	$result = <SEND>;
	print "JOB REMOVE failed: $result\n" if(/^-ERR/);
	}

# Turn off autoflush because the message could be long.
SEND->autoflush(0);

# Tell the other end that we are going to send
# the message.	If the formal user name was specified,
# use a different format of the MESSAGE command.
if($user ne '')
	{ print SEND "MESSAGE $user ($for)\n" }
else
	{ print SEND "MESSAGE $for\n" }

# Send the message text.
print SEND "$msg\n";

print SEND "\nProbable cause: $reason\n" if($reason ne "");

print SEND "\nThe title of this job is \"$title\".\n" if($title ne "");

if($pages > 0)
  {
  if($pages == 1)
	 { print SEND "\nIt is 1 page long." }
  else
	 { print SEND "\nIt is $pages pages long." }
  if($charge ne '')
	 { print SEND "	 You have been charged $charge." }
  print SEND "\n";
  }

if($code == $RESP_ARRESTED || $code == $RESP_STRANDED_PRINTER_INCAPABLE || $code == $RESP_STRANDED_GROUP_INCAPABLE)
  {
  my $heading_sent = 0;
  while($line=<STDIN>)
	{
	if(! $heading_sent)
		{
		print SEND "\nYour job's log file follows:\n";
		print SEND "===================================================================\n";
		$heading_sent = 1;
		}
	chomp $line;
	print SEND "$line\n";
	}
  }

# Mark end of message, flush it, and get the result.
SEND->autoflush(1);
print SEND ".\n";
$result = <SEND>;
print "MESSAGE failed: $result\n" if($result =~ /^-ERR/);

# Close the connexion to pprpopup.
close(SEND);

# Say we did our job since there is no
# recourse of we didn't. :)
exit 0;

# end of file
