#
# mouse:~ppr/src/libscript/pprpopup.pl
# Copyright 1995--2002, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is" without
# express or implied warranty.
#
# Last modified 20 November 2002.
#

#
# Routines needed for communicating with pprpopup.
#

use PPR;
use FileHandle;
use Socket;
my $SOCKADDR="S n a4 x8";

my $dbdir = "$PPR::VAR_SPOOL_PPR/pprpopup.db";

#==============================================================
# Connect STDOUT and STDERR to a log file and connect STDIN
# to /dev/null.
#==============================================================
sub log_stdout_stderr
  {
  my $logfile = shift;
  open(STDOUT, ">> $logfile") || die;
  chmod(0666, $logfile);
  open(STDIN, "< /dev/null") || die;
  open(STDERR, ">&STDOUT") || die;
  select(STDERR);
  $| = 1;
  select(STDOUT);
  $| = 1;
  }

#==============================================================
# Open a TCP/IP connexion to the indicated port and address.
#==============================================================
sub open_connexion
	{
	my($handle, $responder_address) = @_;
	my $magic_cookie = "";
	my ($host, $ip_address, $port);

	if($responder_address =~ /^([^:]\.[^:]+):([0-9]+)$/)
		{
		($host, $port) = ($1, $2);
		if($host =~ /^\d+\.\d+\.\d+\.\d+$/)
			{
			$ip_address = inet_aton($host);
			}
		else
			{
			if( ! defined($ip_address = (gethostbyname($host))[4]) )
				{
				print "open_connexion($handle, \"$responder_address\"): gethostbyname(\"$host\") failed, $!\n";
				return 0;
				}
			}
		}
	else
		{
		my $encoded_responder_address = $responder_address;
		$encoded_responder_address =~ s/([^a-zA-Z0-9\@\.])/sprintf("%%%02X", ord $1)/ge;

		# Look for a registration file.
		if(! open(PPRPOPUP_DB, "<$dbdir/$encoded_responder_address"))
			{
			print "open_connexion($handle, \"$responder_address\"): client not registered\n";
			return 0;
			}

		while(my $line = <PPRPOPUP_DB>)
			{
			if($line =~ /^pprpopup_address=(\d+\.\d+\.\d+\.\d+):(\d+)$/)
				{
				$host = $1;
				$port = $2;
				$ip_address = inet_aton($host);
				next;
				}
			if($line =~ /^magic_cookie=(.+)$/)
				{
				$magic_cookie = $1;
				next;
				}
			}

		close(PPRPOPUP_DB);
		}

	defined $host || die;
	defined $ip_address || die;
	defined $port || die;

	my $proto = (getprotobyname("tcp"))[2];

	if( ! socket($handle, AF_INET, SOCK_STREAM, $proto) )
		{
		print "open_connexion(): socket() failed, $!\n";
		return 0;
		}

	if( ! connect($handle, pack($SOCKADDR, AF_INET, $port, $ip_address)) )
		{
		print "open_connexion($handle, \"$responder_address\"): connect() to ${\inet_ntoa($ip_address)}:$port failed, $!\n";
		close($handle);
		return 0;
		}

	$handle->autoflush(1);

	if($magic_cookie ne "")
		{
		print $handle "COOKIE $magic_cookie\n";
		my $result = <$handle>;
		if($result !~ /^\+OK/)
			{
			print $result;
			close($handle);
			return 0;
			}
		}

	return 1;	# sucess!
	}

1;
