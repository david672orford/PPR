#! /usr/bin/perl
#
# mouse:~ppr/src/pprd/pprd-question.perl
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
# Last modified 7 December 2001.
#

#
# This program will send a response to the Tcl/Tk script "pprpopup".
#

require 5.000;
use lib "?";
require 'pprpopup.pl';

# Set a maximum time this script can run.
alarm(30);

# Split the arguments out into individually named variables.
my($response_responder, $response_address, $response_options, $question, $jobname) = @ARGV;

# Do substitutions in the question.
$question =~ s/%JOBNAME%/$jobname/g;

# Open a connexion to pprpopup
open_connexion(SEND, $response_address) || exit(2);

# Send the message to open the web page.
SEND->autoflush(1);
print SEND "HTML http://localhost:15010/$question 6i 2i\n";

# Wait for a reply to our command.
my $result = '';
while(<SEND>)
    {
    $result .= $_;
    last if(/^(\+OK)|(-ERR)/);
    }

# Close the connexion to pprpopup.
close(SEND);

if($result =~ /^-ERR/)
    {
    print $result;
    exit 2;
    }
else
    {
    exit 1;
    }

# end of file
