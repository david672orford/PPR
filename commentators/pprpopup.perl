#! /usr/bin/perl -w
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
# Last modified 19 November 2002.
#

#
# This program will send a response to the Tcl/Tk script "pprpopup"
# which is likely running on a MS-Windows 95 machine.
#

require 5.000;
use lib "/usr/lib/ppr/lib";
require 'respond.ph';
require 'pprpopup.pl';

# Set a maximum time this script can run.
alarm(30);

# Assign names to the command line parameters.
my($address, $options, $printer, $code, $cooked, $raw1, $raw2, $severity, $canned_message) = @ARGV;

# Open a connexion to pprpopup
open_connexion(SEND, $address) || die;

# Turn off autoflush because the message could be long.
SEND->autoflush(0);

# Tell the other end that we are going to send the message.	 
print SEND "MESSAGE $address\n";

# Send the message text.
if($canned_message ne "")
	{
	print SEND "$canned_message\n";
	}
else
	{
	print SEND "$cooked\n";
	}

# Mark end of message, flush it, and get the result.
{
SEND->autoflush(1);
print SEND ".\n";
my $result = <SEND>;
print "MESSAGE failed: $result\n" if($result =~ /^-ERR/);
}

# Close the connexion to pprpopup.
close(SEND);

# Say we did our job since there is no
# recourse of we didn't. :)
exit 0;

# end of file
