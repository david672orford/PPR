#
# mouse:~ppr/src/printdesk/ATchooser.pm
# Copyright 1995--1999, Trinity College Computing Center.
# Written by David Chappell.
#
# This object implements a Macintosh like AppleTalk chooser.
#
# Last modified 9 August 1999.
#

package PrintDesk::ATchooser;

require 5.003;
require Exporter;
use PrintDesk;

@ISA = qw(Exporter);
@EXPORT = qw();

$debug = 0;

# This is called when the object is created.
# It creates the object with default values.
sub new
  {
  if($debug) { print STDERR "PrintDesk::ATchooser::new()\n"; }

  shift;				# what is this?
  my $main = shift;		# first argument

  my $self = {};

  $self->{main} = $main;

  $self->{type} = "LaserWriter";

  bless $self;
  return $self;
  }

sub set_type
  {
  my $self = shift;
  $self->{type} = shift;
  }

#
# This routine is called when it is time to select
# an AppleTalk device.
#
sub Show
	{
	my $self = shift;
	my $done = 0;
	my $result;

	if($debug) { print STDERR "PrintDesk::ATchooser::Select()\n"; }

	my $window = $self->{window} = $self->{main}->Toplevel();
	$window->title("AppleTalk Chooser");
	$window->iconname("Chooser");

	#---------------------------------------
	# Top left, the zone select box
	#---------------------------------------
	my $left_frame = $window->Frame();
	$left_frame->pack(-side, 'left', -padx, '5m', -pady, '5m',
				-fill, 'both', -expand, 1);

	# Label to show this side selects the zone
	$left_frame->Label(-text, "Zone:")->
		pack(-side, 'top', -anchor, 'nw');

	# A listbox which shows the zones:
	my $zone_scrollbar = $left_frame->Scrollbar(-relief, 'sunken');
	$zone_scrollbar->pack(-side, 'right', -fill, 'y');
	my $zone_listbox = $left_frame->Listbox(-width, 30, -height, 8, -bd, 1,
		-relief, 'sunken', -yscrollcommand, ['set', $zone_scrollbar],
		-setgrid, 1, -exportselection, 0);
	$zone_listbox->pack(-side, 'left', -fill, 'both', -expand, 1);
	$zone_scrollbar->configure(-command, ['yview', $zone_listbox]);

	#--------------------------------------
	# Right, the name select box
	#--------------------------------------
	my $right_frame = $window->Frame();
	$right_frame->pack(-side, 'left', -padx, '5m', -pady, '5m',
				-fill, 'both', -expand, 1);

	# A label which shows that this side selects the entity name
	$right_frame->Label(-text, "Entity:")->pack(-side, 'top', -anchor, 'nw');

	# A list box which shows the entitys in the current zone
	$entity_scrollbar = $right_frame->Scrollbar(-relief, 'sunken');
	$entity_scrollbar->pack(-side, 'right', -fill, 'y');
	my $entity_listbox = $right_frame->Listbox(-width, 30, -height, 8, -bd, 1,
				-relief, 'sunken', -yscrollcommand, ['set', $entity_scrollbar],
				-setgrid, 1, -exportselection, 0);
	$entity_listbox->pack(-side, 'left', -fill, 'both', -expand, 1);
	$entity_scrollbar->configure(-command, ['yview', $entity_listbox]);

	#----------------------------------
	# Bottom, the "OK", "Cancel" frame
	#----------------------------------
	my $button_frame = $window->Frame();
	$button_frame->pack(-side, 'left', -anchor, 'sw');

	# The "OK" button, inside a frame of its own
	# which indicates that it is the default button.
	my $default_border = $button_frame->Frame(-relief, 'sunken', -bd, 1);
	$default_border->pack(-side, 'bottom', -anchor, 'sw', -expand, 1, -padx, '5m', -pady, '2m');
	my $ok_button = $default_border->Button(-text, "OK",
		-command,
		sub{
		if( defined($entity_listbox->curselection()) )
				{ $result=$entity_listbox->get($entity_listbox->curselection()); $done=1; }
		else
				{ $entity_listbox->bell(); }
		});
	$ok_button->pack(-padx, '1m', -pady, '1m', -ipadx, '2m', -ipady, '0m');

	# The "Cancel" button, clear the entity name and exit
	$button_frame->Button(-text, "Cancel",
		-command, sub {undef($result); $done= 1 ;})->
		pack(-side, 'bottom', -anchor, 'sw', -expand, 1, -padx, '5m', -pady, '2m', -ipadx, '2m', -ipady, '0m');

	# Bind a mouse press in the listbox to a function which loads
	# an entity list in the right-hand listbox.
	$zone_listbox->bind("<Button-1>", [
		sub {
				my($w, $zone_listbox, $entity_listbox) = @_;
				my $y = $zone_listbox->XEvent->y;
				$self->{zone} = $zone_listbox->get($zone_listbox->nearest($y));
				search_zone($self, $entity_listbox);
				},
		 $zone_listbox, $entity_listbox]);

	# Swallow double click:
	$zone_listbox->bind("<Double-1>", [sub{}]);

	# Double click on a entity listbox chooses OK.
	$entity_listbox->bind("<Double-1>", [sub{$ok_button->invoke()}]);

	# Return chooses OK.
	$window->bind("<Return>", [sub{$ok_button->invoke();} ]);

	# Save the old focus and grab by asking for references
	# to anonymous subroutines to restore them and then
	# and move the focus to the zone selection window.
	my $old_focus = $window->focusSave();
	my $old_grab = $window->grabSave();
	$window->grab();
	$window->focus();

	# Ask for loading of the zone list:
	load_zones($zone_listbox);

	# If there is no action procedure, wait
	# for the dialog to be complete.
	if( ! defined($self->{action_proc}) )
		{
		$window->waitVariable(\$done);
		$window->grabRelease();
		$window->destroy();
		&$old_focus;
		&$old_grab;
		return defined($result) ? "$result:$self->{type}\@$self->{zone}" : undef;
		}
	}

# This routine fills the zone listbox.
# Since it goes quickly we do not do this
# in the background.
sub load_zones
	{
	my($zones_listbox) = @_;

	open(ZONES, "$PrintDesk::GETZONES |") || die;
	my @zones = <ZONES>;
	close(ZONES);

	my $zone;
	foreach $zone (sort(@zones))
		{
		chomp $zone;
		$zones_listbox->insert('end', $zone);
		}

	}

# This is called every time the selection in the
# zone listbox changes.
sub search_zone
	{
	my($self, $entity_listbox) = @_;
	my $pid;

	if($debug) { print STDERR "Searching zone \"$self->{zone}\"\n"; }

	$entity_listbox->delete(0, 'end');

	# If a search is already in progress,
	if($self->{zone_search_active})
		{
		if($self->{zone_search_active} eq $self->{zone})
			{
			if($debug) { print STDERR "Already searching that zone\n"; }
			return;
			}
		if($debug) { print STDERR "Canceling search of \"$self->{zone_search_active}\" in progress\n"; }
		kill('TERM', $self->{zone_search_pid});
		$entity_listbox->fileevent(ENT, 'readable', "");
		close(ENT); # || die "close failed: $!";
		}

	# Make a note of the fact that we are now searching a zone
	# and of what zone it is.
	$self->{zone_search_active} = $self->{zone};

	# Change the cursor to the watch so that the
	# user will know that we are busy.
	$self->{window}->configure(-cursor, 'watch');

	# Start the nbplkup program with a pipe leading its
	# output back to us.
	$pid = $self->{zone_search_pid} = open(ENT, "-|");

	if($pid == 0)				# child
		{
		exec($PrintDesk::NBP_LOOKUP, "--gui-backend", "=:$self->{type}\@$self->{zone}");
		die;
		}
	elsif(! defined($pid))
		{
		die "Fork() failed";
		}

	# Arrange for an action routine to be called for each line.
	$entity_listbox->fileevent(ENT, 'readable', [\&search_zone_fileevent, $self, ENT, $entity_listbox]);
	}

# This routine is called every time there is a line of data
# available from nbplkup.
sub search_zone_fileevent
	{
	my($self, $file, $entity_listbox) = @_;
	my $line;

	if($line = <$file>)
		{
		chomp $line;
		if($debug) { print STDERR "\"$line\"\n"; }
		if($line =~ /^([0-9]+) ([^:]+)/)
			{
			$entity_listbox->insert($1, $2);
			}
		elsif($line =~ /^([0-9]+)/)
			{
			$entity_listbox->delete($1);
			}
		}
	else
		{
		$entity_listbox->fileevent($file, 'readable', "");
		close($file);
		undef($self->{zone_search_active});
		}

	$self->{window}->configure(-cursor, 'top_left_arrow');
	}

1;

__END__
=head1 NAME

PrintDesk::ATchooser.pm

=head1 SYNOPSIS

$chooser = new PrintDesk::ATchooser($window);
$choice = $chooser->Show();

=head1 DESCIPTION

This widget packs an AppleTalk chooser into the indicated window.  The
method "Show" returns the name of the selected AppleTalk entity, or undef if
the user chooses "Cancel".

=cut
