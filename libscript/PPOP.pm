#
# mouse:~ppr/src/libscript/PPOP.pm
# Copyright 1995--2001 Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appears in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is"
# without express or implied warranty.
#
# Last modified 2 August 2001.
#

=head1 NAME

PPRcontrol

=head1 SYNOPSIS

$control = new PPR::PPOP($queuename);
$control->destroy();
$control->su(I<username>);
$control->proxy_for(I<principal>)
$control->qquery(I<fields...>);
$control->qquery_1job(I<jobname>, I<fields...>);
$control->cancel(I<jobname>, I<jobs...>);
$control->hold(I<jobname>, I<jobs...>);
$control->release(I<jobname>, I<jobs...>);
$control->rush(I<jobname>, I<jobs...>);
$control->move(I<jobname>, I<destname>);
$control->stop();
$control->start();
$control->halt();
$control->get_comment();
$control->get_pstatus();
$control->get_progress();
$control->list_destinations();
$control->list_destinations_comments();
$control->list_destinations_comments_addresses();
$control->list_members();
$control->media();
$control->mount();

=head1 DESCRIPTION

This object can be used to control a PPR queue or printer.

=cut

package PPR::PPOP;

require 5.003;
use IPC::Open2;
use FileHandle;
use Carp;
use PPR;

$references = 0;
$ppop_launched = 0;
$ppop_launched_su_user = "";
$ppop_launched_proxy_for = "";

=head2 PPR::PPOP Constructor

The constructor takes a single string as an argument.  This string is the name
of a PPR printer or group.  It is subsequently used as the default queue name
for certain of the member functions.

=cut

sub new
  {
  shift;

  my $self = {};
  $self->{queue} = shift;
  $self->{su_user} = "";
  $self->{proxy_for} = "";

  defined($self->{queue}) || croak "Queue name parameter missing";

  $references++;
  print STDERR "PPR::PPOP::new(): queue = \"$self->{queue}\", \$references = $references\n";

  bless $self;
  return $self;
  }

=head2 The destroy() Method

The destroy() method should be called when a PPRcontrol object is no
longer needed.  Once all PPR::PPOP objects which the application has
created have been destroyed, B<ppop> will be terminated.

=cut

sub destroy
    {
    my $self = shift;
    defined($self) || croak;

    $references--;
    print STDERR "PPR::PPOP::destroy(): \$references = $references\n";
    if($references == 0)
    	{
	$self->shutdown();
    	}
    }

#
# This function is called to launch a copy of ppop.  It is launched in
# machine-readable mode.  This function is called each time ppop is
# needed.  It does nothing if ppop is already launched because ppop
# remains around until the last instance of this class is destroyed.
#
sub launch
  {
  my $self = shift;
  defined($self) || croak;

  # If ppop is already running but it was launched with a --su switch which
  # isn't correct for this object instance, shut it down so we can launch a
  # new one.  This isn't efficent, but no existing code uses object instances
  # with different user IDs, so this never happens anyway.  We are just
  # being prepared.
  if($ppop_launched && ($self->{su_user} ne $ppop_launched_su_user || $self->{proxy_for} ne $ppop_launched_proxy_for))
    {
    print STDERR "PPR::PPOP::launch(): need shutdown and relaunch\n";
    $self->shutdown();
    }

  if( ! $ppop_launched )
    {
    # I don't remember what this is for.  Perhaps it has something to do
    # with an obscure bug in Perl.
    my $saved_buffer_mode = $|;
    $| = 1;
    print "";

    defined($PPR::PPOP_PATH) || croak;

    # Build the basic command line.
    my @COMMAND = ($PPR::PPOP_PATH, "-M");

    # If the su() method was used, add the --su option.
    if($self->{su_user} ne "")
    	{
	push(@COMMAND, "--su", $self->{su_user});
    	}

    # If the proxy_for() method was used, add the --proxy-for option.
    if($self->{proxy_for} ne "")
	{
	push(@COMMAND, "--proxy-for", $self->{proxy_for});
	}

    # Launch the command.
    #print STDERR "Launching: ", join(" ", @COMMAND), "\n";
    ($rdr, $wtr) = (FileHandle->new, FileHandle->new);
    my $pid = open2($rdr, $wtr, @COMMAND);
    $wtr->autoflush();

    # The first thing it does is print the PPR version number.
    my $junk = <$rdr>;
    $junk =~ /^\*READY\t([0-9.]+)/ || die "ppop not ready";

    print STDERR "PID $pid, PPR version $1\n";

    $ppop_launched = $pid;
    $ppop_launched_su_user = $self->{su_user};

    # Undo whatever that was we did.
    $| = $saved_buffer_mode;

    }
  }

#
# Internal function
#
sub shutdown
  {
  my $self = shift;
  defined($self) || croak;

  # Flush and close the write pipe.
  close($wtr);

  # Drain the read pipe.
  while(<$rdr>)
    {
    }

  # Close the read pipe.
  close($rdr);

  # Get the exit status of the process.
  waitpid($ppop_launched, 0);

  # Clear the process-alive flag.
  $ppop_launched = 0;
  }

#
# Internal function
# Read a response from ppop and return the rows of tab
# delimited columns as a list or array references.
#
sub read_lists
    {
    my @result_rows = ();

    print STDERR "PPR::PPOP::read_lists()\n";

    while(<$rdr>)
	{
	print STDERR $_;
	chomp;
	if(/^\*DONE\t([0-9]+)/)
	    {
	    return undef if($1 != 0);
	    last;
	    }
	if(/^\*/)
	    {
	    croak "Unexpected line from ppop: $_\n";
	    }
	my @columns = split(/\t/, $_, 1000);
	push(@result_rows, \@columns);
	}

    return @result_rows;
    }

#
# Internal function
# Do something to the indicated jobs.
#
# @result_message_lines = do_it($operation, @joblist)
#
sub do_it
    {
    my $self = shift;
    my $exit_code = -1;
    my @output = ();

    $self->launch();

    print STDERR "\$ ppop -M ", join(' ', @_), "\n";
    print $wtr join(' ', @_), "\n";

    while(1)
	{
	defined($_ = <$rdr>) || croak;
	#print STDERR $_;
	chomp;
	if(/^\*DONE\t([0-9]+)/)
	    {
	    $exit_code = $1;
	    last;
	    }
	if(/^\*/)
	    {
	    croak "Unexpected line from ppop: $_\n";
	    }
	push(@output, $_);
	}

    if($exit_code != 0)
	{
	return @output;
	}

    return ();
    }

=head2 The su() Method

The su() method can be called at any time.  It sets a value for use with
the B<ppop --su> switch.  If B<ppop> has already been launched with a different
value for with no B<--su> switch, it will be relaunched.  If an application
has multiple PPR::PPOP objects with different usernames set with
the su() method, then using one then another will result in the restarting
of B<ppop> which will reduce efficiency somewhat.

=cut

sub su
    {
    my $self = shift;
    defined($self->{su_user} = shift) || croak;
    }

=head2 The proxy_for() Method

The proxy_for()N method can be called at any time.  It acts just like the
su() method, except that it sets a value for the B<--proxy-for> switch.

=cut

sub proxy_for
    {
    my $self = shift;
    defined($self->{proxy_for} = shift) || croak;
    }

=head2 The qquery() Method

Get the indicated columns of data for all jobs in the queue.  The column
names are the same as those used by B<ppop qquery>.

=cut

sub qquery
  {
  my $self = shift;

  # Launch ppop if it isn't running yet.
  $self->launch();

  # Send a command to ppop.
  print STDERR "ppop -M qquery $self->{queue}-* ", join(' ', @_), "\n";
  print $wtr "qquery $self->{queue}-* ", join(' ', @_), "\n";

  my @result = ();
  my $i = 0;
  while(<$rdr>)
    {
    chomp;
    if(/^\*DONE\t([0-9]+)/)
	{
	last;
	}
    if(/^\*/)
    	{
	croak "Unexpected line from ppop: $_\n";
    	}
    my @result_fields = split(/\t/, $_, 1000);
    #print STDERR join(', ', @result_fields), "\n";
    $result[$i++] = \@result_fields;
    }

  return @result;
  }

=head2 The qquery_1job Method

Get the indicated columns for a certain job.

=cut

sub qquery_1job
  {
  my $self = shift;
  my $jobname = shift;
  my @result_fields = ();

  defined($jobname) || croak "Missing jobname parameter";

  $self->launch();

  print $wtr "qquery $jobname ", join(' ', @_), "\n";

  while(<$rdr>)
    {
    if($_ =~ /^\*DONE\t([0-9]+)/)
	{
	last;
	}
    chomp;
    @result_fields = split(/\t/, $_);
    }

  return @result_fields;
  }

=head2 The cancel() Method

Cancel the jobs listed in the argument list.

=cut

sub cancel
    {
    my $self = shift;
    my @joblist = @_;
    #print "Cancel: ", join(' ', @joblist), "\n";
    return $self->do_it("cancel", @joblist);
    }

=head2 The hold() Method

Change the state of the jobs in the argument list to "held".

=cut

sub hold
    {
    my $self = shift;
    my @joblist = @_;
    #print "Hold: ", join(' ', @joblist), "\n";
    return $self->do_it("hold", @joblist);
    }

=head2 The release() Method

Release the held jobs name in the argumnet list.

=cut

sub release
    {
    my $self = shift;
    my @joblist = @_;
    #print "Release: ", join(' ', @joblist), "\n";
    return $self->do_it("release", @joblist);
    }

=head2 The rush() Method

Move the jobs in the argument list to the head of the queue.

=cut

sub rush
    {
    my $self = shift;
    my @joblist = @_;
    #print "Rush: ", join(' ', @joblist), "\n";
    return $self->do_it("rush", @joblist);
    }

=head2 The move() Method

Move the job named in the first argument to the queue named in the
second argument.

=cut

sub move
    {
    my $self = shift;
    my $job = shift;
    my $newdest = shift;
    return $self->do_it("move", $job, $newdest);
    }

=head2 The start() Method

Start a printer.  If there is no argument, the printer associated with the
object is started, otherwise the printer named in the argument is started.

=cut

sub start
    {
    my $self = shift;
    my $printer = shift;
    $printer = $self->{queue} unless defined($printer);
    return $self->do_it("start", $printer);
    }

=head2 The stop() Method

Stop a printer.  If there is no argument, the printer associated with the
object is stopt, otherwise the printer named in the argument is stopt.

=cut

sub stop
    {
    my $self = shift;
    my $printer = shift;
    $printer = $self->{queue} unless defined($printer);
    return $self->do_it("stop", $printer);
    }

=head2 The halt() Method

Halt (stop in the middle of the current job) a printer.  If there is no
argument, the printer associated with the object is halted, otherwise the
printer named in the argument is halted.

=cut

sub halt
    {
    my $self = shift;
    my $printer = shift;
    $printer = $self->{queue} unless defined($printer);
    return $self->do_it("halt", $printer);
    }

=head2 The get_comment() Method

Get the comment which describes this alias, group, or printer.

=cut

sub get_comment
    {
    my $self = shift;
    my $printer = shift;
    $printer = $self->{queue} unless defined($printer);

    $self->launch();				# start ppop
    print $wtr "ldest $printer\n";		# send command
    my @result_rows = read_lists();		# read the response lines
    my @result_row1 = @{$result_rows[0]};	# take first response

    return $result_row1[4];			# comment is 4th column
    }

=head2 The get_pstatus() Method

Get the status of the printer.

The return value is a list of array references of this form:
[$name, $status, $retry_number, $retry_countdown, $printer_message1, ...]

=cut

sub get_pstatus
    {
    my $self = shift;
    my $printer = shift;
    $printer = $self->{queue} unless defined($printer);

    $self->launch();

    print $wtr "status $printer\n";

    my @result_rows = read_lists();
    my $row;
    foreach $row (@result_rows)
    	{
	my $status = $row->[1];
	my $job = "";
	my $retry = 0;
	my $countdown = 0;

	# Split out fault engaged into retry number and seconds left.
	if($status =~ /^((fault)|(engaged)) (\d+) (\d+)$/)
	    {
	    ($status, $job, $retry, $countdown) = ($1, "", $4, $5);
	    }

	# If retrying printing, extract the retry count.
	elsif($status =~ /^(printing) (\S+) (\d+)$/)
	    {
	    ($status, $job, $retry, $countdown) = ($1, $2, $3, 0);
	    }

	# Split out various things that halt the job.
	elsif($status =~ /^((canceling)|(seizing)|(stopping)|(halting)) (\S+) (\d+)$/)
	    {
	    ($status, $job, $retry, $countdown) = ($1, $6, $7, 0);
	    }

	# Replace the origional status field with 3 fields
	# containing its split-out values.
	splice(@{$row}, 1, 1, $status, $job, $retry, $countdown);
	}

    return @result_rows;
    }

=head2 The get_progress() Method

Get the job progress for the printer.

=cut

sub get_progress
    {
    my $self = shift;
    my $job = shift;
    $job = $self->{queue} unless defined($job);

    $self->launch();
    print $wtr "progress $job\n";
    my @result_rows = read_lists();
    return @{shift @result_rows};
    }

=head2 The get_alerts() Method

Get the alert message lines.  They are returned as a long string with
embedded newlines.

=cut

sub get_alerts
    {
    my $self = shift;
    my $printer = shift;
    $printer = $self->{queue} unless defined($printer);

    print $wtr "alerts $printer\n";

    my $lines = "";

    while(<$rdr>)
	{
	if($_ =~ /^\*DONE\t([0-9]+)/)
	    {
	    if($1 != 0) { $lines .= "!!! error $1 !!!\n" };
	    last;
	    }
	$lines .= $_;
	}

    return $lines;
    }

=head2 The list_destinations() Method

Return a list of all printers, groups, and aliases.

The return value is a list of array references.  Each array has this form:
[$name, $type, $accepting, $protected]

=cut

sub list_destinations
    {
    my $self = shift;
    my $queue = shift;
    $queue = $self->{queue} unless defined($queue);
    $self->launch();
    print $wtr "ldest $queue\n";
    return read_lists();
    }

=head2 The list_destinations_comments() Method

Return a list of all printers, groups, and aliases.

The return value is a list of array references.  Each array has this form:
[$name, $type, $accepting, $protected, $comment]

=cut

sub list_destinations_comments
    {
    my $self = shift;
    my $queue = shift;
    $queue = $self->{queue} unless defined($queue);
    $self->launch();
    print $wtr "destination-comment $queue\n";
    return read_lists();
    }

=head2 The list_destinations_comments_addresses() Method

Return a list of all printers, groups, and aliases.

The return value is a list of array references.  Each array has this form:
[$name, $type, $accepting, $protected, $comment, $address]

=cut

sub list_destinations_comments_addresses
    {
    my $self = shift;
    my $queue = shift;
    $queue = $self->{queue} unless defined($queue);
    $self->launch();
    print STDERR "ppop -M destination-comment-address $queue\n";
    print $wtr "destination-comment-address $queue\n";
    return read_lists();
    }

=head2 The list_members() Method

List the members this group if this object instances represents a group,
or list the printer if this object represents a printer.

=cut

sub list_members
    {
    my $self = shift;
    my $queue = shift;
    $queue = $self->{queue} unless defined($queue);
    $self->launch();
    print $wtr "status $queue\n";

    my $row;
    my @list = ();
    foreach $row (read_lists())
    	{
	push(@list, $row->[0]);
    	}
    return @list;
    }

=head2

List the bins and mounted media for a printer.

=cut

sub media
    {
    my $self = shift;
    my $printer = $self->{queue};
    $self->launch();
    print $wtr "media $printer\n";
    return read_lists();
    }

=head2

Mounted a medium on a printer's bin.

=cut

sub mount
    {
    my $self = shift;
    my $printer = $self->{queue};
    my $bin = shift;
    my $medium = shift;
    #print "Mount: ", join(' ', $printer, $bin, $medium), "\n";
    return $self->do_it("mount", $printer, $bin, $medium);
    }

1;

