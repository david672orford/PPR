#
# mouse:~ppr/src/printdesk/PPRlistqueue.pm
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

=head1 NAME

PrintDesk::PPRlistqueue

=head1 SYNOPSIS

$list = new PrintDesk::PPRlistqueue(I<$window>, I<$queue>);
$list->Show();
@selected_list = $list->getSelection();
$list->destroy();

=head1 DESCRIPTION

This widget displays a PPR queue listing.

=cut

package PrintDesk::PPRlistqueue;

use PPR::PPOP;
use PrintDesk::PPRstateupdate;

require 5.003;
use PrintDesk;

=item new PrintDesk::PPRlistqueue(I<$window>, I<$queue>)

Create a new queue widget.

=cut
sub new
  {
  my $self = {};
  bless $self;

  shift;								# junk
  $self->{window} = shift;				# frame or toplevel it build it in
  $self->{queue} = shift;				# queue to display

  #print "PrintDesk::PPRlistqueue::new(): \"$self->{queue}\"\n";

  # Define the columns to display, their ppop qquery names,
  # and their widths.
  $self->{columns} = [
		{-title, "Job", -name, "jobname", -width, 15},
		{-title, "User", -name, "for", -width, 15},
		{-title, "Title", -name, "title", -width, 30},
		{-title, "Submitted", -name, "subtime", -width, 9},
		{-title, "Pages", -name, "pages", -width, 4},
		{-title, "Status", -name, "status", -width, 17},
		{-title, "", -name, "explain", -width, 15}
		];

  # Create a jobcontrol object which we will use to
  # get information about jobs.
  $self->{jobcontrol} = new PPR::PPOP($self->{queue});

  return $self;
  }

=item $list->destroy()

Destroy this listqueue widget

=cut
sub destroy
	{
	my $self = shift;
	#print "PrintDesk::PPRlistqueue::destroy()\n";
	$self->{jobcontrol}->destroy();
	$self->{updater}->destroy();
	#$self->{window}->destroy();
	}

#
# Return the job control object for use by the job control
# button box.
#
sub get_jobcontrol
	{
	my $self = shift;
	return $self->{jobcontrol};
	}

#
# This routine is bound to Button-1 or B1-Motion for each
# listbox.	For Button-1 , the drag argument is 0, for B1-Motion
# it is 1.
#
sub parallel_select {
		my $lb = shift;
		my $lbs = shift;
		my $drag = shift;
		my $y = $lb->XEvent->y;
		my $nearest = $lb->nearest($y);

		foreach $box (@{$lbs})
				{
				if(!$drag)
						{
						$box->selectionClear(0, 'end');
						$box->selectionAnchor($nearest);
						}
				$box->selectionSet($nearest);
				$box->see($nearest);
				}
		}

#
# This public routine should be called when it is time
# to display the listbox.
#
sub Show
  {
  my $self = shift;
  my $w = $self->{window};
  my $temp;

  # We build stuff in these variables and store them
  # in $self->{} later.
  my @qquery_columns = ();
  my $listboxes_by_qquery = {};
  my $columns_count = 0;
  my $jobname_column = -1;
  my @listboxes = ();

  # Create a scrollbar to scroll the queue listing:
  $self->{scrollbar} = $w->Scrollbar()->
		pack(-side, 'right', -anchor, 'ne', -fill, 'y');

  # Create the columns:
  foreach $field (@{$self->{columns}})
	{
	# Create a frame for this column and push a label and
	# a divider to the top.
	$temp = $field->{frame} = $w->Frame(
		)->pack(-side, 'left', -anchor, 'nw', -fill, 'both', -expand, 1);
	$field->{titlelabel} = $temp->Label(
		-text => $field->{-title}
		)->pack(-side, 'top', -anchor, 'nw');
	$field->{titledivider} = $temp->Frame(
		-height => 1,
		-background => 'black'
		)->pack(-side, 'top', -anchor, 'nw', -fill, 'x');

	# Create the listbox which displays the rows for this column.
	$field->{listbox} = $temp->Listbox(
		-width => $field->{-width},
		-bd => 0,
		-setgrid => 1,
		-exportselection => 0,
		-background => 'white'
		)->pack(-side, 'left', -anchor, 'nw', -fill, 'both', -expand, 1);

	# Renounce all of the default bindings and
	# bind button on to the selection routine.
	$field->{listbox}->bindtags([$field->{listbox}]);
	$field->{listbox}->bind("<Button-1>",
		[\&parallel_select, \@listboxes, 0]);
	$field->{listbox}->bind("<B1-Motion>",
		[\&parallel_select, \@listboxes, 1]);

	# If this is the first column we have created then we must tie
	# it to the scrollbar so that the thumb will move and
	# change size.	To have all the listboxes move the scrollbar
	# would be wasteful.
	if($#listboxes == -1)
		{ $field->{listbox}->configure(-yscrollcommand, ['set', $self->{scrollbar}]); }

	# Pack a narrow black bar on the right to divide this column
	# from the next one.
	$field->{sidedivider} = $w->Frame(-width, 1, -background, 'black')->
		pack(-side, 'left', -anchor, 'nw', -fill, 'y');

	# We want to note the column number of the jobname column.
	if($field->{'-name'} eq "jobname") { $jobname_column = $columns_count; }

	# Add this column to the list of columns to be
	# retrieved using "ppop qquery".
	push(@qquery_columns, $field->{'-name'});

	# Index this column by its qquery name so that we can find the
	# status and extra columns easily.
	$listboxes_by_qquery->{$field->{'-name'}} = $field->{listbox};

	# Add to list of boxes which will scroll together.
	push(@listboxes, $field->{listbox});

	$columns_count++;
	}

  $self->{scrollbar}->configure(-command,
		[sub {my $listboxes = shift;
		my $box;
		foreach $box (@{$listboxes})
			{
			$box->yview(@_);
			}
		}, \@listboxes]);

  # We must have a jobname column, even if it is hidden.
  # If it is hidden, it is not in $columns_count.
  if($jobname_column == -1)
	{
	push(@qquery_columns, "jobname");
	$jobname_column = $columns_count;
	}

  #
  # Get a list of the current queue contents.
  # The qquery() function returns an array of
  # references to arrays.
  #
  my @answer = $self->{jobcontrol}->qquery(@qquery_columns);

  #
  # Fill in the form with the current queue contents.
  # Also, add each job to the list of jobs which
  # we keep in the array pointed to by $job_list.
  #
  my $job_list = [];
  foreach $entry (@answer)
	{
	push(@$job_list, $entry->[$jobname_column]);
	my $i = 0;
	foreach $field (@{$self->{columns}})
		{
		$field->{listbox}->insert('end', $entry->[$i]);
		$i++;
		}
	}

  #print "\$jobname_column = $jobname_column\n";
  #print "job_list = ", join(' ', @$job_list), "\n";

  $self->{qquery_columns} = \@qquery_columns;
  $self->{listboxes} = \@listboxes;
  $self->{listboxes_by_qquery} = $listboxes_by_qquery;
  $self->{columns_count} = $columns_count;
  $self->{jobname_column} = $jobname_column;
  $self->{job_list} = $job_list;

  # Create an updater object and tell it what routines to
  # call when the queue listing changes.
  my $updater = new PrintDesk::PPRstateupdate($w, $self->{queue});
  $updater->register('add', $self, \&add);
  $updater->register('delete', $self, \&delete);
  $updater->register('newstatus', $self, \&newstatus);
  $updater->register('move', $self, \&move);
  $updater->register('rename', $self, \&rename);
  $self->{updater} = $updater;
  }

#
# This public routine returns a list
# of selected print jobs.
#
sub getSelection
	{
	my $self = shift;
	my $job_list = $self->{job_list};

	my $a_listbox = $self->{listboxes}->[0];
	my @selected_rows = $a_listbox->curselection();

	my $row;
	my @selected_jobs = ();
	foreach $row (@selected_rows)
		{
		push(@selected_jobs, $job_list->[$row]);
		}

	return @selected_jobs;
	}

#
# State update callback:
#
# This routine is called every time a job should be
# added to the listing.
#
sub add
	{
	my($self, $jobname, $rank) = @_;

	#print "Add job $jobname at row $rank\n";

	# Get the columns of data for this job.
	my @columns = $self->{jobcontrol}->qquery_1job($jobname, @{$self->{qquery_columns}});

	# Store the job name in the list of jobs.
	splice(@{$self->{job_list}}, $rank, 0, @columns[$self->{jobname_column}]);

	# Put the columns data in the listboxes.
	my $i = 0;
	foreach $field (@{$self->{columns}})
		{
		$field->{listbox}->insert($rank, $columns[$i]);
		$i++;
		}
	}

#
# State update callback:
#
# This routine is called to remove jobs from the listing.
#
sub delete
	{
	my($self, $jobname) = @_;

	#print "Delete job $jobname\n";

	my $columns = $self->{columns};				# columns of the queue listing
	my $job_list = $self->{job_list};

	my $x = 0;
	my $y;
	foreach $entry (@$job_list)
		{
		if($entry eq $jobname)
			{
			# Delete this job from the job_list array.
			splice(@$job_list, $x, 1);

			# Delete this row from all columns
			my $stop = $self->{columns_count};
			for($y = 0; $y < $stop; $y++)
				{ $columns->[$y]->{listbox}->delete($x); }

			last;
			}
		$x++;
		}
	}

#
# State update callback:
#
# This routine is called every time a job's status changes.
#
sub newstatus
	{
	my($self, $jobname, $status) = @_;

	#print "New status for job $jobname: \"$status\"\n";

	my $job_list = $self->{job_list};

	my $status_listbox = $self->{listboxes_by_qquery}->{status};
	my $extra_listbox = $self->{listboxes_by_qquery}->{extra};

	# If we aren't displaying either of these two columns,
	# there is nothing for us to do.
	if( ! defined($status_listbox) && ! defined($extra_listbox) )
		{ return; }

	my $x = 0;
	foreach $entry (@$job_list)
		{
		if($entry eq $jobname)
			{
			if(defined($status_listbox))
				{
				my $selected = $status_listbox->selectionIncludes($x);
				$status_listbox->delete($x);
				$status_listbox->insert($x, $status);
				if($selected) { $status_listbox->selectionSet($x); }
				}

			if(defined($extra_listbox))
				{
				my($extra) = $self->{jobcontrol}->qquery_1job($jobname, "extra");
				my $selected = $status_listbox->selectionIncludes($x);
				$extra_listbox->delete($x);
				$extra_listbox->insert($x, $extra);
				if($selected) { $status_listbox->selectionSet($x); }
				}
			last;
			}
		$x++;
		}
	}

#
# State update callback:
#
# This is called every time a job's position in the
# queue listing changes.  This is the result of
# of using the ppop rush command.
#
sub move
	{
	my($self, $jobname, $newrank) = @_;
	my $job_list = $self->{job_list};

	#print "Move job $jobname to row $newrank\n";

	my $x = 0;
	foreach $entry (@$job_list)
		{
		if($entry eq $jobname)
			{
			my $stop = $self->{columns_count};
			my $listboxes = $self->{listboxes};
			for($y = 0; $y < $stop; $y++)
				{
				my $listbox = $listboxes->[$y];;
				my $data = $listbox->get($x);
				$listbox->delete($x);
				$listbox->insert($newrank, $data);
				}
			splice(@$job_list, $x, 1);
			splice(@$job_list, $newrank, 0, $jobname);
			last;
			}
		$x++;
		}
	}

#
# State update callback:
#
# This is called every time a job's name changes.  This will only
# be called if we are viewing "all".
#
sub rename
	{
	my($self, $oldname, $newname) = @_;

	#print "Change name of job $oldname to $newname\n";

	# Obtain a reference to the listbox which contains the jobnames.
	my $jobname_listbox = $self->{listboxes_by_qquery}->{jobname};

	# If we are not displaying the jobname, we don't care.
	if( ! defined($jobname_listbox) )
		{ return; }

	my $x = 0;
	foreach $entry (@{$self->{job_list}})
		{
		if($entry eq $oldname)
			{
			$jobname_listbox->delete($x);
			$jobname_listbox->insert($x, $newname);
			last;
			}
		$x++;
		}
	}

1;
