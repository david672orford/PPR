#
# mouse:~ppr/src/libscript/tcpserver.pl
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
# Last modified 17 January 2002.
#

#
# This function can be used to provide a standalone mode for a server that is
# normally run from Inetd.
#
# It should be called with a single argument which is either a port name from
# /etc/services or a port number.  This function will return in the children
# with STDIN and STDOUT connected to the client.  The parent will never return.
#
sub tcpserver
    {
    my $standalone_port = shift;

    # If the port isn't numberic, look it up in /etc/services.
    if($standalone_port !~ /^\d+$/)
    	{
	($standalone_port) = (getservbyname($standalone_port, "tcp"))[2];
	defined($standalone_port) || die "Unknown port $standalone_port";
    	}

    # Craete the server socket.
    socket(SERVER, PF_INET, SOCK_STREAM, getprotobyname("tcp")) || die "socket() failed, $!";
    setsockopt(SERVER, Socket::SOL_SOCKET, Socket::SO_REUSEADDR, 1) || die "setsockopt() failed, $!";
    my $my_address = sockaddr_in($standalone_port, INADDR_ANY);
    bind(SERVER, $my_address) || die "bind() failed, $!";
    listen(SERVER, SOMAXCONN) || die "listen() failed, $!";

    # This works in Linux.
    $SIG{CHLD} = 'IGNORE';

    while(1)
	{
	accept(CLIENT, SERVER) || die $!;
	my $pid = fork();

	if(!defined $pid)
	    {
	    print STDERR "Fork failed, $!\n";
	    close CLIENT;
	    next;
	    }

	# if parent,
	if($pid)
	    {
	    close CLIENT || die "close() of connection socket failed, $!";
	    next;
	    }

	# child
	close SERVER || die $!;
	open(STDIN, "<&CLIENT") || die $!;
	open(STDOUT, ">&CLIENT") || die $!;
	last;
	}

    }

1;

