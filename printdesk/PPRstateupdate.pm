#
# mouse:~ppr/src/printdesk/PPRstateupdate.pm
# Copyright 1995--2003, Trinity College Computing Center.
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
# Last modified 28 March 2003.
#

=head1 NAME PrintDesk::PPRstateupdate

=head1 SYNOPSIS

$updater = new PrintDesk::PPRstateupdate(I<$window>, I<$queue>);
$updater->register(I<event>, I<$object>, I<method>);
$updater->unregister(B<event>);
$updater->destroy();

=head1 DESCRIPTION

This class can be used to monitor the PPR queue and printers.

=over 4

=cut

package PrintDesk::PPRstateupdate;

require 5.003;
require Exporter;
use Fcntl;
use PrintDesk;

@ISA = qw(Exporter);
@EXPORT = qw();

$debug = 0;

@instances = ();
$instances_count = 0;
$unique_id = 0;
$tail_status_launched = 0;

=item $updater = new PrintDesk::PPRstateupdate(I<$widget>, I<$queue>)

This function is used to create a new object for monitoring PPR printer
status.

=cut
sub new
    {
    shift;
    my $self = {};
    bless $self;
    $self->{widget} = shift;
    $self->{queue} = shift;

    print STDERR "PrintDesk::PPRstateupdate::new(): widget=$self->{widget}, queue=\"$self->{queue}\" " if($debug);

    # Create a unique id so that we can later locate this instance
    # in the array @instances.  This is necessary because its possition
    # will change if other objects of this type created before it are
    # destroyed.
    $self->{unique_id} = $unique_id;
    $unique_id++;

    # Push this instance onto a list which will be searched whenever
    # an event is detected.
    push(@instances, $self);

    # Start tail_status if it is not already started.
    if(! $tail_status_launched)
	{
	open(UPDATES, "$PrintDesk::TAIL_STATUS_PATH |") || die;
	fcntl(UPDATES, &F_SETFL, &O_NONBLOCK);
	$self->{widget}->fileevent(UPDATES, 'readable', \&handler);
	$tail_status_launched = 1;
	}

    # Keep track of how many we have.
    $instances_count++;

    print STDERR "\$instances_count = $instances_count\n" if($debug);

    return $self;
    }

=item $updater->destroy()

Destroy the object.  All of its event handlers are unregistered.

=cut
sub destroy
    {
    my $self = shift;
    print STDERR "PrintDesk::PPRstateupdate::destroy(): " if($debug);

    my $x;
    print STDERR "\tSearching for this instance (unique_id = $self->{unique_id}) ... " if($debug);
    for($x=0; $x < $instances_count; $x++)
    	{
	if($instances[$x]->{unique_id} == $self->{unique_id})
	    {
	    print STDERR "match at instance $x\n" if($debug);
	    splice(@instances, $x, 1);
	    last;
	    }
    	}
    print STDERR "no match !!!\n" if($debug && $x >= $instances_count);

    $instances_count--;
    print STDERR "\$instances_count = $instances_count\n" if($debug);
    }

=item $updater->register(I<$event>, I<$object>, I<$method>)

Register a hander for a specific type of event.  Method I<$method> will be
called in object I<$object> whenever the event occurs.  The parameters
passed to the callback function depend on the event type.

=cut
sub register
    {
    my($self, $event, $object, $method) = @_;
    print STDERR "PPRstateupdate::register(\$self=$self, \$event=$event, \$object=$object, \$method=$method)\n" if($debug);
    $self->{$event} = [$object, $method];
    }

=pod

The valid event types are as follows:

=over 4

=cut

#
# Internal function
#
# This function is called whenever data is received over the pipe
# from tail_status.
#
sub handler
    {
    my $instance;
    my $finfo;

    #
    # Read update messages from pprd or pprdrv.
    #
    while(<UPDATES>)
	{
	chomp;
	print STDERR "> $_\n" if($debug);

	#=====================
	# pprd messages
	#=====================

=item queue

New job

=cut
	if($_ =~ /^JOB ([^ ]+) ([0-9]+) ([0-9]+)/o)
	    {
	    my($jobname, $rank1, $rank2) = ($1, $2, $3);
	    foreach $instance (@instances)
		{
		if(defined($finfo=$instance->{add}))
		    {
		    if($instance->{queue} eq "all")
			{
			&{$finfo->[1]}($finfo->[0], $jobname, $rank1);
			}
		    elsif($jobname =~ /([^-]+)[-]/o && $1 eq $instance->{queue})
			{
			&{$finfo->[1]}($finfo->[0], $jobname, $rank2);
			}
		    }
		}
	    next;
	    }

=item delete

Job removed

=cut
	if($_ =~ /^DEL ([^ ]+)/)
	    {
	    my $jobname = $1;
	    foreach $instance (@instances)
		{
		if(defined($finfo=$instance->{'delete'}))
		    {
		    if($instance->{queue} eq "all" || ($jobname =~ /([^-]+)[-]/o && $1 eq $instance->{queue}))
			{
			&{$finfo->[1]}($finfo->[0], $jobname);
			}
		    }
		}
	    next;
	    }

=item newstatus

Job status changed

=cut
	if($_ =~ /^JST ([^ ]+) (.*)/)
	    {
	    my ($jobname, $status) = ($1, $2);
	    foreach $instance (@instances)
		{
		if(defined($finfo=$instance->{newstatus}))
		    {
		    if($instance->{queue} eq "all" || ($jobname =~ /([^-]+)[-]/o && $1 eq $instance->{queue}))
			{
			&{$finfo->[1]}($finfo->[0], $jobname, $status);
			}
		    }
		}
	    next;
	    }

	# Move a job from one queue to another.  We will issue
	# appropriate add, delete, or rename calls.
	if($_ =~ /^MOV ([^-]+)-([^ ]+) ([^ ]+) ([0-9]+)/)
	    {
	    my($olddest, $id, $newdest, $rank2) = ($1, $2, $3, $4);

	    foreach $instance (@instances)
		{
		if(defined($finfo=$instance->{"rename"}) && $instance->{"queue"} eq "all")
		    {
		    &{$finfo->[1]}($finfo->[0], "$olddest-$id", "$newdest-$id");
		    }
		if(defined($finfo=$instance->{"add"}) && $instance->{"queue"} eq $newdest)
		    {
		    &{$finfo->[1]}($finfo->[0], "$newdest-$id", $rank2);
		    }
		if(defined($finfo=$instance->{"delete"}) && $instance->{"queue"} eq $olddest)
		    {
		    &{$finfo->[1]}($finfo->[0], "$olddest-$id");
		    }
		}
	    }

# Rush a job.  (Move it to the head of the queue.)
=item move

Change a job's position in the queue

=cut
	if($_ =~ /^RSH ([^-]+)-([^ ]+)/)
	    {
	    my($dest, $id) = ($1, $2);
	    my $finfo;

	    foreach $instance (@instances)
		{
		if(defined($finfo=$instance->{move}))
		    {
		    if($instance->{queue} eq "all" || $instance->{queue} eq $dest)
			{
			&{$finfo->[1]}($finfo->[0], "$dest-$id", 0);
			}
		    }
		}
	    }

=item pstatus

Printer status change

=cut
	if($_ =~ /^PST (\S+) (.+)/)
	    {
	    my($printer, $status, $retry, $countdown) = ($1, $2, 0, 0);

	    if($status =~ /^fault (\d+) (\d+)/)
		{
		($retry, $countdown) = ($1, $2);
		if($retry != -1)
		    { $status = "fault"; }
		else
		    { $status = "fault, no auto retry"; }
		}

	    elsif($status =~ /^(printing \S+) (\d+)/)
		{ ($status, $retry) = ($1, $2); }

	    foreach $instance (@instances)
		{
		if(defined($finfo=$instance->{pstatus}))
		    {
		    if($instance->{queue} eq "all" || $instance->{queue} eq $printer)
			{
			&{$finfo->[1]}($finfo->[0], $printer, $status, $retry, $countdown);
			}
		    }
		}
	    }

	# add more pprd messages here

	#=====================
	# pprdrv messages
	#=====================

# PGSTA myprn 5
=item ppages

Start of page transmission

=cut
	if($_ =~ /^PGSTA (\S+) (\d+)/)
	    {
	    foreach $instance (@instances)
		{
		if(defined($finfo=$instance->{ppages}))
		    {
		    if($instance->{queue} eq "all" || $instance->{queue} eq $1)
			{
			&{$finfo->[1]}($finfo->[0], $1, $2);
			}
		    }
		}
	    }

# PGFIN myprn 3
=item pfpages

Page hit output tray

=cut
	elsif($_ =~ /^PGFIN (\S+) (\d+)/)
	    {
	    foreach $instance (@instances)
		{
		if(defined($finfo=$instance->{pfpages}))
		    {
		    if($instance->{queue} eq "all" || $instance->{queue} eq $1)
			{
			&{$finfo->[1]}($finfo->[0], $1, $2);
			}
		    }
		}
	    }

# BYTES myprn 10020 25040
=item pbytes

Update on total bytes sent

=cut
	elsif($_ =~ /^BYTES (\S+) (\d+) (\d+)/)
	    {
	    foreach $instance (@instances)
		{
		if(defined($finfo=$instance->{pbytes}))
		    {
		    if($instance->{queue} eq "all" || $instance->{queue} eq $1)
			{
			&{$finfo->[1]}($finfo->[0], $1, $2, $3);
			}
		    }
		}
	    }

# STATUS myprn off line
=item pmessage

Printer status message

=cut
	elsif($_ =~ /^STATUS (\S+) (.*)/)
	    {
	    foreach $instance (@instances)
		{
		if(defined($finfo=$instance->{pmessage}))
		    {
		    if($instance->{queue} eq "all" || $instance->{queue} eq $1)
			{
			&{$finfo->[1]}($finfo->[0], $1, $2);
			}
		    }
		}
	    }

# EXIT myprn EXIT_PRINTED
=item exit

Printer status message

=cut
	elsif($_ =~ /^PEXIT (\S+) (.*)/)
	    {
	    foreach $instance (@instances)
		{
		if(defined($finfo=$instance->{pexit}))
		    {
		    if($instance->{queue} eq "all" || $instance->{queue} eq $1)
			{
			&{$finfo->[1]}($finfo->[0], $1, $2);
			}
		    }
		}
	    }

	# add more pprdrv messages here

	} # end of state_update loop

    } # end of handler()

=back

=item $updater->unregister(I<$event>)

Remove an event handler.

=cut
sub unregister
    {
    my($self, $event) = @_;
    print STDERR "PPRstateupdate::unregister(\$self=$self, \$event=$event)\n" if($debug);
    undef($self->{$event});
    }

=back

=HEAD1 BUGS

When the first instance of this class in instantiated, a copy of tail_status
is launched.  However, it is not shut down when the last instance is
destroyed.

=cut

1;
