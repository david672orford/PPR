#
# mouse:~ppr/src/filters_typeset/Print/Objects.pl
# Copyright 1995--1999 Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is"
# without express or implied warranty.
#
# Last modified 28 July 1999.
#

#
# An object: anything that can go on a horizontal
# or vertical list.
#

package Print::Object;

sub new
    {
    shift;
    my $self = {};
    return bless $self;
    }

#
# Return the length of an object.  The length is its height above the
# baseline plus its depth below.  This has a special application somewhere.
#
sub GetLength
    {
    return new PT(0);
    }

#
# Return the width of an object.  A generic object has no width.
# This will be overridden in derived classes.
#
sub GetWidth
    {
    return new PT(0);
    }

#
# Return the height of an object.
#
sub GetHeight
    {
    return new PT(0);
    }

#
# Return the depth of an object.
#
sub GetDepth
    {
    return new PT(0);
    }

#
# Return the stretch of an object.
#
sub GetStretch
    {
    return new PT(0);
    }

#
# Return the shrink of an object.
#
sub GetShrink
    {
    return new PT(0);
    }

#
# For debugging
#
sub debug
    {
    my $self = shift;
    my $offset = shift;
    print STDERR leader($offset), ref($self), "\n";
    }

#
# An hbox or vbox.
# At this point we mean a rather
# generic sort of box.
#
# SP width;    	# width of box
# SP height;	# height above baseline
# SP depth;	# drop below baseline
# SP shift;	# amount to shift box, right for hboxes, up for vboxes
#
package Print::Object::Box;
@ISA qw(Print::Object);

sub new
    {
    shift;
    my($iwidth, $iheight, $idepth) = @_;

    my $self =
	{
	width => $iwidth,
	height => $iheight,
	depth => $idepth,
	shift => 0
	};

    return bless $self;
    }

sub SetHeight
    {
    my $self = shift;
    $self->{height} = shift;
    }

sub SetDepth
    {
    my $self = shift;
    $self->{depth} = shift;
    }

sub SetWidth
    {
    my $self = shift;
    $self->{width} = shift;
    }

sub SetShift
    {
    my $self = shift;
    $self->{shift} = shift;
    }

sub GetHeight
    {
    my $self = shift;
    return $self->{height};
    }

sub GetDepth
    {
    my $self = shift;
    return $self->{depth};
    }

sub GetWidth
    {
    my $self = shift;
    return $self->{width};
    }

sub GetShift
    {
    my $self = shift;
    return $self->{shift};
    }

#
# This prints the dimensions of a box.
#
sub debug
    {
    my $self = shift;
    my $offset = shift;

    $self->SUPER::debug($offset);

    print STDERR leader($offset), "width = ", $self->{width}, "\n";
    print STDERR leader($offset), "height = ", $self->{height}, "\n";
    print STDERR leader($offset), "depth = ", $self->{depth}, "\n";
    print STDERR leader($offset), "shift = ", $self->{shift}, "\n";
    }

#
# A character box.
# This is an elaboration of a generic box.
#
package Print::Object::CBox;
@ISA = qw(Print::Object::Box);

sub new
    {
    shift;
    my($ifontid, $isize, $icharname, $icharcode, $iwidth, $iheight, $idepth) = @_;

    my $self = new Print::Object::Box($iwidth, $iheight, $idepth);
    $self->{fontid} = $ifontid;
    $self->{size} = $isize;
    $self->{charname} = $icharname;
    $self->{charcode} = $icharcode;

    return bless $self;
    }

sub GetFontID { return (shift)->{fontid} }
sub GetCharCode { return (shift)->{charcode} }
sub GetCharName { return (shift)->{charname} }
sub GetSize { return (shift)->{size} }

#
# This adds character information for character boxes.
#
sub debug
    {
    my $self = shift;
    my $offset = shift;

    $self->SUPER::debug($offset);

    print STDERR leader($offset), "fontid = ", $self->{fontid}, "\n";
    print STDERR leader($offset), "charname = \"", $self->{charname}, "\"\n";
    }

# A horizontal or vertical box
# consisting of other boxes.
package Print::Box::SuperBox;

sub new
    {
    shift;
    my($iwidth, $iheight, $idepth, $imembers) = @_;

    my $self = new Print::Box($iwidth, $iheight, $idepth);
    $self->{members} = $imembers;

    return bless $self;
    }

sub GetMembers
    {
    my $self = shift;
    return $self->{members};
    }

sub GetNumMembers
    {
    my $self = shift;
    return $#{$self->{members}} + 1;
    }

#
# This prints the members of a hbox or vbox one by one.
#
sub debug
    {
    my $self = shift;
    my $offset = shift;

    $self->SUPER::debug($offset);

    print STDERR leader($offset), "nummembers=", $self->{nummembers}, "\n";

    my $x;
    my $members = $self->{members}
    for($x=0; $x < $#{$members}; $x++)
	{
	print STDERR leader($offset), "member ", $x + 1, "\n";
	$members->[$x]->debug($offset + 1);
	}
    }

# A rule.  Another elaboration of
# the idea of a box.
package Print::Object::Rule;
@ISA = qw(Print::Object::Box;

sub new
    {
    shift;
    my ($iwidth, $iheight, $idepth, $igray) = @_;

    my $self = new Print::Box($iwidth, $iheight, $idepth);
    $self->{graylevel} = $igray;

    return bless $self;
    }

sub getgray
    {
    my $self = shift;
    return $self->{graylevel};
    }

sub debug
    {
    my $self = shift;
    my $offset = shift;

    $self->SUPER::debug($offset);
    print STDERR leader($offset), "graylevel=", $self->{graylevel}, "\n";
    }

# A penalty.  Not derived from a box at all.
package Print::Object::Penalty
@ISA = qw(Print::Object);

sub new
    {
    shift;
    my $self = new Print::Object();
    my $self->{'value' => shift};
    return bless $self;
    }

sub GetValue
    {
    my $self = shift;
    return $self->{value};
    }

sub debug
    {
    my $self = shift;
    my $offset = shift;
    print STDERR leader($offset), ref($self), "\n";
    print STDERR leader($offset), "value=", $self->{value}, "\n";
    }

#
# A kern.  This is a sort of a box, but because
# it has only one demension, we will not derive
# it from Box.
#
# SP width;		# Amount to move right (neg is left)
# int is_explicit;	# was this an explicit kern or did it come from AFM or TFM file
#
package Print::Object::Kern;
@ISA = qw(Print::Object);

sub new
    {
    shift;
    my($width, $is_explicit) = @_;
    if(ref($width) ne 'SCALAR')
    	{
	my $Arg = $width;
	$width = $Arg->GetWidth();
	$is_explicit = $Arg->GetExplicit();
    	}
    my $self = new::Print::Object();
    defined($self->{width} = $width) || die;
    defined($self->{is_explicit} = $is_explicit) || die;
    }

sub GetWidth { return (shift)->{width} }
sub GetExplicit { return (shift)->{is_explicit} }

sub debug
    {
    my $self = shift;
    my $offset = shift;
    $self->SUPER::debug($offset);
    print STDERR leader($offset), "width=", $self->{width}, "\n";
    }

# This is like a kern, but we will not derive it from Kern
# because Kern has the extra member "is_explicit".
class Glue
	{
	SP owidth;	# original width
	SP width;	# width of glue blob
	SP stretch;	# maximum desirable stretch
	SP shrink;	# maximum allowed shrink

	public:

	Glue(void)
	    {
	    width = stretch = shrink = 0;
	    }
	Glue(const SP& iwidth, const SP& istretch, const SP& ishrink)
	    {
	    owidth = width = iwidth;
	    stretch = istretch;
	    shrink = ishrink;
	    }
	Glue(const Glue& Arg)
	    {
	    owidth = Arg.owidth;
	    width = Arg.width;
	    stretch = Arg.stretch;
	    shrink = Arg.shrink;
	    }
	void Set(const SP& iwidth, const SP& istretch, const SP& ishrink)
	    {
	    owidth = width = iwidth;
	    stretch = istretch;
	    shrink = ishrink;
	    }
	SP GetWidth(void) { return width; }
	SP GetStretch(void) { return stretch; }
	SP GetShrink(void) { return shrink; }
	void SetWidth(const SP& nwidth) { width=nwidth; }
	void debug(int offset) const;
	} ;

#
# Debuging routine to print the width, stretch, and shrink of glue.
#
void Glue::debug(int offset) const
	{
	cerr << leader(offset) << "owidth=" << owidth << "\n";
	cerr << leader(offset) << "width=" << width << "\n";
	cerr << leader(offset) << "stretch=" << stretch << "\n";
	cerr << leader(offset) << "shrink=" << shrink << "\n";
	} # end of Glue::debug()

# Discretionary
class Discretionary
	{
	SuperBox *pre;
	SuperBox *post;
	SuperBox *no;

	public:

	Discretionary(SuperBox *ipre, SuperBox *ipost, SuperBox *ino)
		{ pre = ipre; post = ipost; no = ino; }

	# Get a pointer to one of the components of the
	# discretionary break.
	SuperBox *GetPre(void) { return pre; }
	SuperBox *GetPost(void) { return post; }
	SuperBox *GetNo(void) { return no; }

	# Return a pointer to one of the components of the
	# discretionary break and set that component to null
	# so that its storage will not be de-allocated when
	# the discretionary break is destroyed.
	SuperBox *ClaimPre(void) { SuperBox *p=pre; pre = 0; return p; }
	SuperBox *ClaimPost(void) { SuperBox *p=post; post = 0; return p; }
	SuperBox *ClaimNo(void) { SuperBox *p=no; no = 0; return p; }

#
# Debuging routine to print out a discretionary break.
#
void Discretionary::debug(int offset) const
	{
	cerr << leader(offset) << "pre-break:\n";
	if(pre)
		pre->debug(offset+1);

	cerr << leader(offset) << "post-break:\n";
	if(post)
		post->debug(offset+1);

	cerr << leader(offset) << "no-break:\n";
	if(no)
		no->debug(offset+1);
	} # end of Discretionary::debug()

# Breakpoint.  This is used when breaking paragraphs into
# lines and documents into pages.
class Breakpoint
	{
	int via;			# index of previous breakpoint we got here from
	int location;		# offset into the horizontal list
	ISP bphsize;		# hsize to reach this bp
	int hbox_start;		# hlist offset of preposed hbox start
	ISP boxes;			# width of boxes
	ISP glue;			# natural glue width
	ISP maxstretch;		# maximum amount total glue can stretch
	ISP maxshrink;		# maximum amount total glue can shrink
	int golden;			# true if it is in finally selected chain
	int total_demerits;	# total demerits so far, counting these
	int left_disbreak;	# broken discretionary at left side
	int right_disbreak;	# broken discretionary at right side

	public:

	# Copy from another Breakpoint.
	Breakpoint(Breakpoint& Arg)
	    {
	    via		= Arg.via;
	    location	= Arg.location;
	    bphsize	= Arg.bphsize;
	    hbox_start	= Arg.hbox_start;
	    boxes	= Arg.boxes;
	    glue	= Arg.glue;
	    maxstretch	= Arg.maxstretch;
	    maxshrink	= 0;
	    golden	= Arg.golden;
	    total_demerits	= Arg.total_demerits;
	    left_disbreak	= Arg.left_disbreak;
	    right_disbreak	= Arg.right_disbreak;
	    }

	# Initial setup.
 	Breakpoint(ISP ibphsize=0, int ivia=0, int ihbox_start=0)
 	    {
	    bphsize	= ibphsize;
 	    via		= ivia;
 	    location	= 0;
	    hbox_start	= ihbox_start;
 	    boxes = glue = maxstretch = maxshrink = 0;
 	    golden	= false;
 	    total_demerits = 0;
 	    left_disbreak = right_disbreak = false;
 	    }

 	void Set(ISP ibphsize=0, int ivia=0, int ihbox_start=0)
 	    {
	    bphsize	= ibphsize;
 	    via		= ivia;
 	    location	= 0;
	    hbox_start	= ihbox_start;
 	    boxes = glue = maxstretch = maxshrink = 0;
 	    golden	= false;
 	    total_demerits = 0;
 	    left_disbreak = right_disbreak = false;
 	    }

	# Add a fixed width box to the preposed hbox.
	void AddBox(ISP width)
	    {
	    boxes += width;
	    }

	# Add glue to the preposed hbox.
 	void AddGlue(Glue *newglue)
	    {
	    glue += newglue->GetWidth();
	    maxstretch += newglue->GetStretch();
	    maxshrink += newglue->GetShrink();
	    }

	# Compute badness based upon the hsize which was specified
	# when the breakpoint structure was created and the calls
	# we have made to AddBox() and AddGlue().
	int Badness();

	# Compute basic penalty.
	int Demerits(int badness, int linepenalty, int penalty);

	# Get where it is from.
	int GetVia() { return via; }

	# Get and set total demerits.  The recorded total demerits
	# is used when computing the total demerits of a string
	# of breakpoints which could divide the paragraph into lines.
	int GetTotalDemerits() { return total_demerits; }
	void SetTotalDemerits(int t) { total_demerits=t; }

	# Get and set location at which the break occurs.
	# The location is specified as the first horizontal
	# list index which indicates a member which is _not_
	# part of the preposed line.
	int GetLocation() { return location; }
	void SetLocation(int l) { location=l; }

	# Get and set golden.  If a breakpoint is ``golden''
	# it will be part of the final paragraph.
	int IsGolden(void) { return golden; }
	void SetGolden(int igolden) { golden=igolden; }

	# Return the amount of total stretch we have.
	SP GetMaxStretch(void) { return maxstretch; }

	# Return the total amount of available shrink.
	SP GetMaxShrink(void) { return maxshrink; }

	# Say if hbox is overfull.
	int Overfull(void) { return ((boxes + glue - maxshrink) > bphsize); }

	# Get and set hbox_start.
	int GetHboxStart(void) { return hbox_start; }
	void SetHboxStart(int i_hbox_start) { hbox_start=i_hbox_start; }

	# These are used in setting glue.
	SP GetBPhsize(void) { return bphsize; }
	SP GetBoxes(void) { return boxes; }
	SP GetGlue(void) { return glue; }

	# These are used for discretionary breaks at the begining or end.
	void SetLeftDisBreak(int n) { left_disbreak=n; }
	void SetRightDisBreak(int n) { right_disbreak=n; }
	int GetLeftDisBreak(void) { return left_disbreak; }
	int GetRightDisBreak(void) { return right_disbreak; }
	} ;

#
# Return the badness of a breakpoint
#
# See the TeXbook, page 97.
#
int Breakpoint::Badness()
	{
	SP needed_gluesize;		# how much glue is needed to make line fit hbox
	SP change;				# amount glue must change from natural size
	double ratio;			# ratio of change to allowed change
	int badness;

	# How wide do we want the glue to be?
	needed_gluesize = bphsize - boxes;

	# How much does that differ from its natural size?
	change = needed_gluesize - glue;

	if(change > 0)				# If box must be stretched,
		{
		if(maxstretch == 0)		# If no stretch available,
			{					# i.e., box is underfull,
			return 10001;		# return infinity.
			}
		if(maxstretch.GetInfinity())	# If there is any infinitely stretchable glue,
			{
			return 0;					# then the badness is zero.
			}
		ratio = (double)change.GetValue() / (double)maxstretch.GetValue();
		}
	else if(change < 0)			# If box must be shrunk,
		{
		if( (maxshrink==0) || ((change + maxshrink) < 0) )
			{					# If insufficient shrink available, box is overfull,
			return 10001;		# return infinite badness.
			}
		ratio = (double)(-change.GetValue()) / (double)maxshrink.GetValue();
		}
	else						# If no change needed,
		{						# ratio is zero.
		ratio = 0.0;
		}

	# If computing the badness would result in a floating point
	# overflow, return 10,000 which is not maximum non-infinite
	# badness.
	if(ratio > 10.0)
		return 10000;

	# Compute the badness
	badness = (int) (ratio * ratio * ratio * 100.0);

	# If the badness is less than 10,000, return badness,
	# if it is more than or equal to 10,000, return 10,000.
	if( badness <= 10000 )
		return badness;
	else
		return 10000;
	} # end of Breakpoint::Badness()

#
# Compute the demerits.
# This does not include extra demerits for such
# things as visualy incompatible lines.
#
# See The TeXbook, page 98.
#
int Breakpoint::Demerits(int badness, int linepenalty, int penalty)
	{
	int tb;

	tb=badness+linepenalty;

	if(penalty > 0)
		return (tb * tb) + (penalty * penalty);
	else if(penalty <= -10000)
		return (tb * tb);
	else
		return (tb * tb) - (penalty * penalty);
	} # end of Breakpoint::Demerits()

# end of file

