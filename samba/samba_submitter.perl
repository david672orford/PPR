#! /usr/bin/perl -w
#
# mouse.trincoll.edu:~ppr/src/misc/samba_submitter.perl
# Copyright 1996, 1998, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is" without
# express or implied warranty.
#
# Last modified 17 September 1998.
#

# System configuration values:
$HOMEDIR = "?";
$TEMPDIR = "?";

use FileHandle;
use Socket;
unshift(@INC, "$HOMEDIR/lib");
require 'pprpopup.pl';

# Turning this on can cause incorrect operation.
#$DEBUG = 1;

# Values derived from system configuration values:
$PPR = "$HOMEDIR/bin/ppr";
$LOGFILE = "$TEMPDIR/samba_submitter.log";

# Assign names to our arguments.
my($CLIENT_NBNAME, $CLIENT_IP, $CLIENT_PORT, $CLIENT_OS, $USER, $PRINTER, $FILE) = @ARGV;

if($DEBUG)
  {
  log_stdout_stderr($LOGFILE);
  print "CLIENT_NBNAME = \"$CLIENT_NBNAME\", CLIENT_IP=\"$CLIENT_IP\", CLIENT_PORT=$CLIENT_PORT, USER = \"$USER\", PRINTER = \"$PRINTER\", FILE = \"$FILE\"\n";
  }

my @for;
my @responder;

# Make a connexion to the client's pprpopup.
print "Attempting to open connexion...\n";
if( open_connexion(CLIENTCON, "$CLIENT_IP:$CLIENT_PORT") )
    {
    # Demand a user name.
    print "Sending request for user name...\n" if($DEBUG);
    sleep(1);
    print CLIENTCON "USER\r\n";

    # Wait for an answer.
    my $answer = <CLIENTCON>;
    $answer =~ s/\s+$//;
    print "\$answer = \"$answer\"\n" if($DEBUG);

    close(CLIENTCON);

    my $username;

    # If we got a user name,
    if( $answer =~ /^\+OK (.+)$/ )
	{
	$username = $1;
	}

    # If the user canceled the job, dump it.
    elsif( $answer =~ /^-ERR/ )
	{
	print "Discarding job\n" if($DEBUG);
	unlink($FILE);
	exit(0);
	}

    else
	{
	print "Received null answer!\n";
	$username = '?';
	}

    @responder = ('-m', 'pprpopup', '-r', "$CLIENT_IP:$CLIENT_PORT");
    @for = ('-f', $username);
    }

# Since pprpopup is not running on the client computer, we will
# so this best we can.  This includes instructing PPR to use
# the "samba" responder.
else
    {
    @responder = ('-m', 'samba',
    		'-r', "$CLIENT_NBNAME-$CLIENT_IP",
		'--responder-options', "os=$CLIENT_OS");
    @for = ('-f', $USER);
    }

# Replace ourself with ppr:
exec($PPR, '-d', $PRINTER, '-e', 'responder', '-w', 'log',
	@responder, @for,
	'-X', "$CLIENT_NBNAME\@samba",
	'-C', '', '-U', '-I', $FILE);

die "Exec failed!";

# end of file

