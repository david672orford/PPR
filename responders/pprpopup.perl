#! /usr/bin/perl
#
# mouse:~ppr/src/responders/pprpopup.perl
# Copyright 1995--2001, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software is provided "as is" without express or
# implied warranty.
#
# Last modified 20 December 2001.
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
# want to get a chance to see what they look like.  The output will
# appear on ppr's stdout or in the pprd log file.
#print "\$for = \"$for\"\n";
#print "\$addr = \"$addr\"\n";
#print "\$msg = \"$msg\"\n";
#print "\$code = $code\n";
#print "\$jobid = \"$jobid\"\n";
#print "\$extra = \"$extra\"\n";
#print "\$title = \"$title\"\n";
#print "\$time = $time\n";
#print "\$reason = \"$reason\"\n";
#print "\$pages = $pages\n";

# Undo the cryptic arrest reason encoding.
$reason =~ s/,/, /g;
$reason =~ s/[|]/ /g;

# If a user is specified in the address,
# break the address into user and machine.
my $user = "";
if($addr =~ /^([^\@]+)\@.+$/)
    {
    $user = $1;
    }

# Open a connexion to pprpopup
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
# the message.  If the formal user name was specified,
# use a different format of the MESSAGE command.
if($user ne '')
    { print SEND "MESSAGE $user ($for)\n" }
else
    { print SEND "MESSAGE $for\n" }

# Send the message text.
print SEND "$msg\n";

print SEND "\nProbable cause: $reason\n" if($reason ne "");

print SEND "\nThe title of this job is \"$title\".\n" if($title ne "");

if($pages ne "?")
  {
  if($pages == 1)
     { print SEND "\nIt is 1 page long." }
  else
     { print SEND "\nIt is $pages pages long." }
  if($charge ne '')
     { print SEND "  You have been charged $charge." }
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
print "MESSAGE failed: $result\n" if(/^-ERR/);

# Close the connexion to pprpopup.
close(SEND);

# Say we did our job since there is no
# recourse of we didn't. :)
exit 0;

# end of file
