#! @PERL_PATH@ -w
#
# mouse:~ppr/src/misc/samba_submitter.perl
# Copyright 1995--2005, Trinity College Computing Center.
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
# Last modified 17 January 2005.
#

use lib "@PERL_LIBDIR@";
use FileHandle;
use Socket;
use PPR;
require 'pprpopup.pl';

# Turning this on can cause incorrect operation.
#$DEBUG = 1;

# Values derived from system configuration values:
$LOGFILE = "$PPR::LOGDIR/samba_submitter.log";

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
	print CLIENTCON "USER\n";

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
# so this best we can.	This includes instructing PPR to use
# the "samba" responder.
else
	{
	@responder = ('-m', 'samba',
				'-r', "$CLIENT_NBNAME-$CLIENT_IP",
				'--responder-options', "os=$CLIENT_OS");
	@for = ('-f', $USER);
	}

# Replace ourself with ppr:
exec($PPR::PPR_PATH, '-d', $PRINTER, '-e', 'responder', '-w', 'log',
		@responder, @for,
		'-X', "$CLIENT_NBNAME\@samba",
		'-C', '', '-U', '-I', $FILE);

die "Exec failed!";

# end of file

