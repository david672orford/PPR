#
# mouse:~ppr/src/www/cgi_run.pl
# Copyright 1995--2004, Trinity College Computing Center.
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
# Last modified 26 May 2004.
#

require 'cgi_data.pl';

#===============================================================
# Run a command.  The output is HTML escaped and sent to the
# web browser.  Since it sends output to the browser, you will
# not want to run it in an onnext handler.
#===============================================================
sub run
	{
	my @command_list = @_;

	# Perl 5.8.0 spews warnings if exec() arguments are tainted.
	run_detaint(\@command_list);

	# Print the command we are about to execute.
	print '$ ';
	run_print(@command_list);
	print "\n";

	my $pid;
	my $result = 0;

	if($pid = open(RUNHANDLE, "-|"))	# parent
		{
		while(<RUNHANDLE>)
			{
			print html($_);
			}
		if(!close(RUNHANDLE))
			{
			$result = 1;
			}
		}
	elsif(defined($pid))				# child
		{
		# Make sure errors go the the web page rather than
		# to the server error log.
		open(STDERR, ">&STDOUT");

		# If the full path to the program is specified, clear PATH to
		# avoid problems with tainted PATHs.
		$ENV{PATH} = "" if($_[0] =~ m#^/#);

		# Replace this process with the program to be run.
		exec(@command_list);

		# We mustn't use die here as it would be caught by eval.
		exit(255);
		}
	else								# failure
		{
		print "Can't fork!\n";
		$result = 255;
		}

	return $result;
	}

sub run_pipeline
	{
	my $i = 0;
	my $handle;
	my $previous_handle;
	foreach my $command_list (@_)
		{
		# Perl 5.8.0 spews warnings if exec() arguments are tainted.
		run_detaint($command_list);

		# Print the command we are about to execute.
		if($i == 0)
			{
			print '$ ';
			}
		else
			{
			print '  | ';
			}
		run_print(@$command_list);
		if($i == $#_)
			{
			print "\n";
			}
		else
			{
			print " \\\n";
			}

	    $handle = "RUNHANDLE$i";
		my $pid;
	
		if(!defined($pid = open($handle, "-|")))
			{
			print "Can't fork!\n";
			return 255;
			}
		elsif($pid == 0)					# child
			{
			open(STDERR, ">&STDOUT");
			if($i == 0)
				{
				open(STDIN, "</dev/null");
				}
			else
				{
				open(STDIN, "<&$previous_handle") 
				}
			$ENV{PATH} = "";
			exec(@$command_list);
			exit(255);
			}

		$previous_handle = $handle;
		$i++;
		}

	print STDERR "\$handle = $handle\n";
	while(<$handle>)
		{
		print;
		}
	my $result = 0;
	while(--$i >= 0)
		{
	    $handle = "RUNHANDLE$i";
		if(!close($handle))
			{
			$result++;
			}
		}

	return $result;
	}

#===============================================================
# Run a command and capture the output with a pipe connected 
# to the indicated handle.	For example:
#
# opencmd(JOBS, "ppop", "list", "all");
# while(<JOBS>)
#	 {
#	 print;
#	 }
# close(JOBS);
#
# Since no output is sent to the browser, this can be run from
# inside an onnext handler.
#===============================================================
sub opencmd
	{
	my $handle = shift;
	my @command_list = @_;
	my $stderr_fate = ">&STDOUT";

	if($command_list[0] =~ /^2>(.+)$/)
		{
		$stderr_fate = ">$1";
		shift @command_list;
		}

	# Perl 5.8.0 spews warnings if exec() arguments are tainted.
	run_detaint(\@command_list);

	my $pid = open($handle, "-|");

	return 0 if(! defined($pid));		# fork failed

	return 1 if($pid != 0);				# if parent

	# Keep stderr output out of the server error log.
	open(STDERR, $stderr_fate) if($stderr_fate ne ">&STDERR");

	# If possible, clear the PATH to avoid problems with
	# tainted PATHs.
	$ENV{PATH} = "" if($_[0] =~ /^\//);

	# This is the child.  Exec the program we want.	 The exec() is in 
	# a block by itself to suppress a Perl warning.
	{ exec(@command_list); }

	# We must actually execute something because if we don't, then this
	# copy of Perl will dump its buffers.
	exec("/bin/echo", "exec(\"" . join('", "', @command_list) . "\") failed: $!");

	# Don't use die!
	exit 255;
	}

#===============================================================
# Run a command.  It it fails, call die with its output as the
# error message.  Since this sends output to the web browser,
# it should not be run from inside an onnext handler.
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
		print '$ ';
		run_print(@_);
		print "\n";
		print html($result);
		print "</pre>\n";
		die "external command failed: $error\n";
		}
	}

#===============================================================
# Break a string into words more-or-less as a shell would.
# The return value is a Perl list.  This code is not correct,
# but it handles most things that aren't silly or unlikely
# for the way we are using it.
#===============================================================
sub shell_parse
	{
	my $text = shift;
	my @list = ();
	while($text =~ m/\s* ( 				# possible leading space
			((\S*?)"([^"]*)"(\S*)) 		# something non-space ($2), double quote, something ($3), not double quote, double quote, possibly something non-space ($3)
			| ((\S*?)'([^']*)'(\S*)) 	# same as above but for single quote
			| (\S+) ) 					# kist something nonspace and non-empty  
			\s*/gx)						# possible trailing space
		{
		if(defined($2)) { push(@list, $3 . $4 . $5) }
		elsif(defined($6)) { push(@list, $7 . $8 . $9) }
		elsif(defined($10)) { push(@list, $10) }
		else { die }
		}
	return @list;
	}

#===============================================================
# This detaints all but the first member of the array that is
# passed to it.	 Perl 5.8.0 spews warnings if any of the
# arguments to exec() are tainted.	Note that we are just 
# suppressing the warning since we consider it to be a nusance
# warning (since we aren't exec()ing a shell).
#===============================================================
sub run_detaint
	{
	my $array = shift;
	for(my $i = 1; $i < scalar @$array; $i++)
		{
		@$array[$i] =~ /^(.*)$/;
		@$array[$i] = $1;
		}
	}

#===============================================================
# This is an internal function.	 Print the command in HTML.
# We assume we are in a <PRE> environment.
#
# You will probably want to call it like this:
# 	print '$ ';
# 	run_print(@command);
# 	print "\n";
# This provides a pseudo prompt and closes the line.  This is 
# not done for you by run_print() because run_pipeline() uses
# it to print partial commands.
#===============================================================
sub run_print
	{
	my @command_list = @_;

	# Remove the path from the program.	 It is distracting.
	$command_list[0] = $1 if($command_list[0] =~ m#^(?:/[^/\s]*)*/([^/\s]+)$#);

	my $total_length = 0;

	foreach my $arg (@command_list)
		{
		# If the argument isn't tiny and it would cause the line to become 
		# too long, start a new indented line.
		if($total_length > 4 && ($total_length + 1 + length($arg)) >= 70)
			{
			print " \\\n    ";
			$total_length = 4;
			}
		# Otherwise, just print a space to separate it from 
		# the previous one.
		elsif($total_length > 0)
			{
			print ' ';
			$total_length++;
			}

		# If it has funny characters or spaces, print it quoted
		# (and count two characters for the two quote marks).
		if($arg !~ /^[-_0-9a-zA-Z\/=]+$/)
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
	}
	
1;

