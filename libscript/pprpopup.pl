#
# mouse:~ppr/src/libppr/pprpopup.pl
# Copyright 1995--1999, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is" without
# express or implied warranty.
#
# Last modified 18 November 1999.
#

#
# Routines needed for communicating with pprpopup.
#

# Stuff needed for TCP/IP:
use FileHandle;
use Socket;
my $SOCKADDR="S n a4 x8";

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
  my($handle, $ADDRESS) = @_;

  if($ADDRESS !~ /^([^:]+):([0-9]+)$/)
    {
    print "open_connexion(): address syntax incorrect\n";
    return 0;
    }

  my($HOST, $PORT) = ($1, $2);

  my $proto = (getprotobyname("tcp"))[2];

  if( ! socket($handle, AF_INET, SOCK_STREAM, $proto) )
    {
    print "open_connexion(): socket() failed, $!\n";
    return 0;
    }

  if( ! defined($address = (gethostbyname($HOST))[4]) )
    {
    print "open_connexion($handle, \"$ADDRESS\"): gethostbyname() failed, $!\n";
    return 0;
    }

  if( ! connect($handle, pack($SOCKADDR, AF_INET, $PORT, $address)) )
    {
    print "open_connexion($handle, \"$ADDRESS\"): connect() failed, $!\n";
    close($handle);
    return 0;
    }

  $handle->autoflush(1);
  return 1;	# sucess!
  }

1;
