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
# Last modified 26 March 2003.
#

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

# Internal function
#
# This function is called whenever data is received over the pipe
# from tail_status.
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

	#
	# pprd messages:
	#

	# New job
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

	# Job removed
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

	# Job status changed
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

	# Printer status change
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

	#
	# pprdrv messages:
	#

	# Start of page transmission
	# PGSTA myprn 5
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

	# Page hit output tray
	# PGFIN myprn 3
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

	# total bytes sent
	# BYTES myprn 10020 25040
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

	# Printer status message
	# STATUS myprn off line
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

	# add more pprdrv messages here

	} # end of state_update loop

    } # end of handler()

#
# This function is used to create a new object for monitoring PPR printer
# status.  The member function register()
#
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
	$self->{widget}->fileevent(UPDATES, 'readable', \&handler);
	fcntl(UPDATES, &F_SETFL, &O_NONBLOCK);
	$tail_status_launched = 1;
	}

    # Keep track of how many we have.
    $instances_count++;

    print STDERR "\$instances_count = $instances_count\n" if($debug);

    return $self;
    }

#
# Destroy the object.  All of its event handlers are unregistered.
#
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

#
# Register a hander for a specific type of event.
#
sub register
    {
    my($self, $event, $object, $method) = @_;
    print STDERR "PPRstateupdate::register(\$self=$self, \$event=$event, \$object=$object, \$method=$method)\n" if($debug);
    $self->{$event} = [$object, $method];
    }

#
# Remove an event handler.
#
sub unregister
    {
    my($self, $event) = @_;
    print STDERR "PPRstateupdate::unregister(\$self=$self, \$event=$event)\n" if($debug);
    undef($self->{$event});
    }

1;

__END__
=head1 NAME PrintDesk::PPRstateupdate

=head1 SYNOPSIS

$updater = new PrintDesk::PPRstateupdate($window, $queue);
$updater->register("add", $object, $method);
$updater->register("delete", $object, $method);
$updater->register("newstatus", $object, $method);
$updater->register("rename", $object, $method);
$updater->register("move", $object, $method);
$updater->register("pstatus", $object, $method);
$updater->register("pmessage", $object, $method);
$updater->register("pbytes", $object, $method);
$updater->register("pfpages", $object, $method);
$updater->destroy();

=head1 DESCRIPTION

This class can be used to monitor the PPR queue and printers.

=cut
