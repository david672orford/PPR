#
# mouse:~ppr/src/www/cgi_run.pl
# Copyright 1995--2002, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software is provided "as is" without express or
# implied warranty.
#
# Last modified 5 April 2002.
#

#===============================================================
# Print the command in HTML.  We assume we are in a <PRE>
# environment.
#===============================================================
sub run_print
    {
    print '$';
    my $total_length = 1;
    foreach $arg (@_)
    	{
	if($total_length > 4 && ($total_length + length($arg)) >= 70)
	    {
	    print " \\\n    ";
	    $total_length = 4;
	    }
	else
	    {
	    print ' ';
	    $total_length++;
	    }

	if($arg !~ /^[-_0-9a-zA-Z\/]+$/)
	    {
	    print html("\"$arg\"");
	    $total_length += 2;
	    }
	else
	    {
	    print html($arg);
	    }

	$total_length += length($arg);
    	}
    print "\n";
    }
    
#===============================================================
# Run a command.  The output is sent to the web browser after
# HTML escaping it.  The real and effective user ids are
# reversed when the command is run.
#===============================================================
sub run
    {
    # Print the command we are about to execute.
    run_print @_;

    my $pid;
    my $result;

    if($pid = open(RUNHANDLE, "-|"))	# parent
	{
	while(<RUNHANDLE>)
	    {
	    s/&/&amp;/g;		# order is important here
	    s/</&lt;/g;
	    s/>/&gt;/g;
	    print;
	    }
	close(RUNHANDLE);
	$result = $?
	}
    elsif(defined($pid))		# child
    	{
	# Make sure errors go the the web page rather than
	# to the server error log.
	open(STDERR, ">&STDOUT");

	# Swap the real and effective user ids.
	($<,$>) = ($>,$<);

	# If possible, clear the PATH to avoid problems with
	# tainted PATHs.
	$ENV{PATH} = "" if($_[0] =~ /^\//);

	# Replace this process with the program to be run.
	exec @_;

	# Catch exec failures
	die "exec failed";
    	}
    else				# failure
    	{
	print "Can't fork!\n";
	$result = 255;
    	}

    return $result;
    }

#===============================================================
# Run a command and capture the output with a pipe.
#===============================================================
sub opencmd
    {
    my $handle = shift;

    my $pid = open($handle, "-|");

    return 0 if(! defined($pid));	# fork failed

    return 1 if($pid != 0);		# if parent

    # Make sure errors go the the web page rather than
    # to the server error log.
    open(STDERR, ">&STDOUT");

    # If possible, clear the PATH to avoid problems with
    # tainted PATHs.
    $ENV{PATH} = "" if($_[0] =~ /^\//);

    # This is the child.  Exec the program we want.  The exec() is in 
    # a block by itself to suppress a Perl warning.
    { exec(@_); }

    # We must actually execute something because if we don't, then this
    # copy of Perl will dump its buffers.
    exec("/bin/echo", "exec(\"" . join('", "', @_) . "\") failed: $!");
    die;
    }

#===============================================================
# Run a command and die with its output if it fails.
#===============================================================
sub run_or_die
    {
    opencmd(RUN_OR_DIE, @_);
    my $result = "";
    while(my $line = <RUN_OR_DIE>)
	{
	$result .= $line;
	}
    if(!close(RUN_OR_DIE))
	{
	my $error = $! ? $! : ("exit code " . ($? >> 8));
	print "<pre>\n";
	run_print @_;
	print html($result);
	print "</pre>\n";
	die "external command failed: $error\n";
	}
    }

#===============================================================
# Break a string into words more-or-less as a shell would.
#===============================================================
sub shell_parse
    {
    my $text = shift;
    my @list = ();
    while($text =~ m/\s*(((\S*)"([^"]*)"(\S*))|((\S*)'([^']*)'(\S*))|(\S+))\s*/g)
    	{
	if(defined($2)) { push(@list, $3 . $4 . $5) }
	elsif(defined($6)) { push(@list, $7 . $8 . $9) }
	elsif(defined($10)) { push(@list, $10) }
	else { die }
    	}
    return @list;
    }

1;

