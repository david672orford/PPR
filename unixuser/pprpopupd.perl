#! /usr/bin/perl
#
# mouse:~ppr/src/misc/pprpopupd.perl
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
# Last modified 25 August 1999.
#

# This is filled in when this script is installed.
$XWINBINDIR="?";
$VAR_SPOOL_PPR="?";

# The name of the console logging device:
$CONSOLE = "/dev/console";

# Set XAUTHORITY for xconsole.
$ENV{XAUTHORITY} = "$VAR_SPOOL_PPR/run/Xauthority";

# Turn off stdout buffering so client
# will get our replies.
$| = 1;

# Should we print extra messages?
$DEBUG = 0;

# Read and dispatch commands.
while(<STDIN>)
    {
    s/\s+$//;

    # This command sends a message to a user.  The
    # argument is the informal designation of the user's
    # identity.  For example, if the user set with SETUSER
    # is "chappell", then the argument for this command
    # might be "David Chappell".
    if(/^MESSAGE (.+)$/)
    	{
	my $parms;
	my $user;
	my $fullname;
	my $display;

	# If possible, split the user specification into
	# username and fullname.
    	$parms = $1;
	if($parms =~ /^(\S+) \(([^\)]+)\)$/)
	    {
	    $user = $1;
	    $fullname = $2;
	    }
	elsif($parms =~ /^(\S+)$/)
	    {
	    $user = $1;
	    $fullname = $1;
	    }
	else		# old PC format
	    {
	    $user = "*";
	    $fullname = $parms;
	    }

	# PC format MESSAGE command.  No really good thing
	# to do with it.
	if($user eq '*')
	    {
	    print "console\n" if($DEBUG);
	    if(! open(P, ">$CONSOLE"))
	    	{
		copy_message('');
	    	print "-ERR can't open $CONSOLE, $!\n";
	    	next;	# get next command
	    	}
	    print P "Message for $fullname:\n\n";
	    }

	# Is the user on the X display?
	elsif(defined($display = x_logged_in($user)))
	    {
	    print "xmessage\n" if($DEBUG);
	    if(! open(P, "| $XWINBINDIR/xmessage -display $display -geometry +100+100 -title \"Message for $fullname\" -bg skyblue -fg black -default okay -file -"))
	    	{
		copy_message('');
	    	print "-ERR can't exec xmessage, $!\n";
	    	next;	# get next command
	    	}
	    }

	# All else fails, use write command.
	elsif(getpwnam($user))
	    {
	    print "write\n" if($DEBUG);
	    if(! open(P, "| write $user"))
		{
		copy_message('');
		print "-ERR can't exec write, $!\n";
		next;	# get next command
		}
	    print P "Message for $fullname:\n\n";
	    }

	else
	    {
	    copy_message('');
	    print "-ERR user \"$user\" unknown\n";
	    next;
	    }

	# Copy the message text to the command.
	copy_message(P);
	if(! close(P))
	    { print "-ERR command failed, $!\n" }
	else
	    { print "+OK message accepted\n" }
    	}

    elsif(/^QUIT/)
    	{
    	print "+OK closing connection\n";
    	last;
    	}

    else
    	{
    	print "-ERR unrecognized command\n";
    	}
    }

exit 0;

#
# If the indicated user is logged on to an X display,
# return the display name.
#
# Right now this only works for the console display.
# It figures that the recipient is logged onto the
# first display on the local machine if he owns /dev/console.
#
sub x_logged_in
    {
    my $user = shift;
    my $console_owner_uid;
    my $console_owner;

    if(defined( ($console_owner_uid = (stat($CONSOLE))[4]) ))
    	{
	if(defined($console_owner = (getpwuid($console_owner_uid))[0]))
	    {
	    if($console_owner eq $user)
	    	{
	    	return ":0.0";
	    	}
	    }
	}

    return undef;
    }

# Copy the message body from stdin to the
# indicated file handle.
sub copy_message
    {
    my $outfile = shift;

    while(<STDIN>)
	{
	s/\s+$//;
	last if($_ eq '.');
	if($outfile) { print $outfile $_, "\n" }
	}
    }

# end of file

