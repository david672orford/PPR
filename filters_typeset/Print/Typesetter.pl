#
# mouse:~ppr/src/filters_typeset/Print/Typesetter.pl
# Copyright 1995--1999, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appears in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is"
# without express or implied warranty.
#
# Last modified 28 July 1999.
#

# This is used to produce output which helps to
# determine if memory leaks are taking place.
$DEBUG_LEAKS = 1;

# If this is defined, we will use floating point rather than
# integer arithmetic in certain places.
$USE_FP = 1;

# Print debugging lines when determining the final
# size of "glue".  Remember that glue is a space which
# may stretch or shrink by a specified amount in the
# interests of attractive page composition.
$DEBUG_GLUESET = 1;

# Print debugging information when things are added to
# and removed from lists.
$DEBUG_LISTS = 1;

# Additional linebreak debugging
$DEBUG_LINEBREAK = 1;

# Useful dimension constants.
$zeropt = new Print::SP(0);
$fil = new Print::SP(65536, 1);
$fill = new Print::SP(65536, 2);
$filll = new Print::SP(65536, 3);

# Possible modes for the typesetter.  This is a
# subset of TeX's modes.
#enum Mode {horizontal, vertical, restricted_horizontal, restricted_vertical};

#
# The typesetter state which is saved and restored by grouping.
# the class "Typesetter" inherits this class.
#
#class TeXState
#	{
#	protected:
#
#	# TeX parameters, general
#	SP hsize;				# horizontal size of line
#	SP vsize;				# vertical size of page body
#	SP maxdepth;			# maximum depth of bottom box
#
#	# TeX character spacing parameters
#	Glue spaceskip;			# inter-word spacing
#	Glue xspaceskip;		# inter-sentence spacing
#	Glue parfillskip;		# paragraph finishing glue
#
#	# TeX line spacing parameters
#	Glue baselineskip;		# target glue between lines
#	Glue lineskip;			# alternate interline glue
#	SP lineskiplimit;		# value which triggers use of lineskip
#	int prevdepth;			# depth of previous box on vertical list, must be int for TeX compatibility
#
#	# TeX line breaking parameters
#	int pretolerance;			# first pass badness tolerance
#	int tolerance;				# final badness tolerance
#	int hyphenpenalty;			# penalty for breaking at a hyphen
#	int exhyphenpenalty;		# penalty for descretionary with empty pre-break text
#	int linepenalty;			# extra penalty for each additional line
#	int adjdemerits;			# extra demerits for visual incompatibility
#	int doublehyphendemerits;	# extra demerits for consecutive hypenation
#	int finalhyphendemerits;	# extra demerits for hyphenating second to last line
#	int tracingparagraphs;		# true if tracing line breaking
#	int looseness;				# amount to adjust total number of lines
#
#	# TeX page breaking parameters
#	int clubpenalty;
#	int widowpenalty;
#	Glue topskip;
#	int tracingpages;
#
#	# TeX page breaking temporary parameters
#	ISP pagetotal;
#	SP pagestretch;
#	SP pageshrink;
#	SP pagegoal;			# Current goal height.
#	SP pagedepth;			# Value of maxdepth at start of page.
#	int insertpenalties;		# The integer ``q''.
#	int best_so_far;		# <- This and
#	int best_cost;			# <- this are my additions.
#	SP best_change;			# <- These are
#	SP best_pageshrink;		# <- mine
#	SP best_pagestretch;		# <- too.
#	} ;

# We will use one of these objects for each active typesetter
package Print::Typesetter;

#	int newvlist;		# size of new unrestricted vertical lists
#	int newrvlist;		# size of new restricted vertical lists
#	int newhlist;		# size of new unrestricted horizontal lists
#	int newrhlist;		# size of new restricted horizontal lists
#
#	int hlist_size;		# horizontal list
#       int vlist_size;         # vertical list
#
#       Object *hlist;          # current horizontal list
#       Object *vlist;          # current vertical list
#
#       int hlist_index;        # number of items on hlist
#       int vlist_index;        # number of items on vlist
#
#       int bplist_size;                # size of breakpoint list
#       Breakpoint *bplist;             # current breakpoint list
#       int bpcount;                    # number of breakpoints currently stored in the list
#
#       SuperBox *dis_pre;              # temporary storage for generating
#       SuperBox *dis_post;             # discretionary breaks
#       SuperBox *dis_no;
#       int dis_stage;
#
#       CharNameList charset;   # current character set
#
#       Mode mode;                              # current mode
#       struct  {                               # a stack for saving modes
#               Mode mode;
#               Object *list;
#               int list_size;
#               int list_index;
#               } stack_mode[STACKSIZE_MODE];
#       int stackptr_mode;      # the pointer to next free entry
#
#       Font *fonts[MAX_FONTS]; # the currently loaded fonts
#       int fonts_index;        # the number of loaded fonts
#       int current_font;       # the font for new characters
#       SP current_size;        # size of current font

# Default output routine.  It simply calls ShipOut()
# (which this class inherits from the PostScript
# class).
void Output(Object *obj)
    {
    print STDERR "Typesetter::Output() (default version)\n";
    ShipOut(obj);
    obj->Clear();
    delete obj;
    }

#
# The constructor for a Typesetter class.
# Sets up a new typesetter.
#
sub new
    {
    shift;
    my $charset_file = shift;

    my $self =
	{
	vlist => [],
	bplist => [],
	mode => "vertical",		# initially in vertical mode
	stackptr_mode => 0,		# no saved values

	# We are not building a discretionary break.
	dis_stage => 0,

	# Load our character set.
	charset => LoadFile($charset_file),

    # Set up the fonts.
    fonts_index = 0;                                # none loaded right now.
    current_font = -1;                              # none selected
    current_size = PT(10);                  # start with 10 point
    SetFontList(fonts);                             # let PostScript class know

    # Initialize TeX parameters, general
    hsize = IN(6.5);                        # horizontal size
    vsize = IN(9);                          # vertical size

    # Initialize Tex line breaking parameters
    pretolerance=100;                       # first pass badness tolerance
    tolerance=200;                          # final badness tolerance
    hyphenpenalty=50;                       # penalty for breaking at a hyphen
    exhyphenpenalty=50;                     # penalty for descretionary with empty pre-break text
    linepenalty=10;                         # extra penalty for each additional line
    adjdemerits=10000;                      # extra demerits for visual incompatibility
    doublehyphendemerits=10000;     # extra demerits for consecutive hypenation
    finalhyphendemerits=5000;       # extra demerits for hyphenating second to last line
    looseness=0;                            # amount to adjust total number of lines
    tracingparagraphs=0;            # true if tracing line breaking

    # Initialize TeX paragraph parameters
    parfillskip.Set(zeropt, fil, zeropt);
    baselineskip.Set( PT(12), PT(3), PT(1) );
    lineskip.Set( PT(3), PT(1), PT(1) );
    lineskiplimit = PT(2);
    spaceskip.Set( PT(6), PT(2), PT(1) );
    xspaceskip.Set( PT(12), PT(9), PT(3) );
    prevdepth = -10000;                     # top of page sentinal value

    # Initialize TeX page breaking parameters;
    maxdepth = PT(2.2);
    tracingpages = 0;
    }

#
# Debug routine to dump the typesetter's current state.
#
void Typesetter::debug()
	{
	cerr << "Vertical list:\n";
	for(int x=0; x < vlist_index; x++)
		vlist[x].debug(1);

	cerr << "\n";

	cerr << "Horizontal list:\n";
	for(int x=0; x < hlist_index; x++)
		hlist[x].debug(1);
	} # end of Typesetter::debug()

#
# Switch to vertical mode.
# This routine should be called whenever we want to
# add vertical material, leaving horizontal mode if we
# are in it.  It is not called when executing EndParagraph().
#
# If we are already in vertical mode or restricted vertical mode,
# this routine does nothing.
#
# private:
void Typesetter::switch_to_vertical(void)
	{
	# Does this require us to abandon a horizontal mode?
	if(mode == (Mode)horizontal)
		EndParagraph();

	# Vertical glue, kerns, etc. can't be inserted in restricted
	# horizontal mode, only in vboxes in restricted horizontal mode.
	else if(mode == (Mode)restricted_horizontal)
		throw("Vertical material is not allowed in restricted horizontal mode\n");
	} # end of Typesetter::switch_to_vertical()

#
# Contribute material to the current vertical list.
#
# Page breaking occurs in this function.  That is to say,
# if you contribute enough material, a page may be removed
# from the top of the vertical list and sent to the output
# file.
#
# You should probably switch to vertical mode before
# calling this, however it is not strictly necessary;
# in fact, EndParagraph() does not return to the enclosing
# mode until this routine has been used to contribute the
# entire paragraph to the vertical list.
#
# private:
void Typesetter::contribute_vertical(OType type, void *ptr)
	{
	# We will compare vlist_index which indicates the next
	# free position in the vertical list array to vlist_size
	# which indicates the size of that array.  If they are equal,
	# then vlist_index points beyond the end of the array, it is full.
	if(vlist_index == vlist_size)
		throw("Vertical list has overflowed");

	# Is this the first item to be contributed to the list?
	if(vlist_index == 0)
		{
		if(tracingpages)
			cerr << "%% goal height=" << vsize << ", max depth=" << maxdepth << "\n";

		pagegoal = vsize;		# save the vsize at the start of the page
		pagedepth = maxdepth;
		pagetotal = 0;			# no boxes or glue accumulated yet
		pagestretch = 0;
		pageshrink = 0;
		best_so_far = 0;
		best_cost = 100001;		# infinity
		}

	# Add the object as the next item in the vertical list.
	vlist[vlist_index].Set(type, ptr);

	# Add the height and stretch of the object
	# to the totals for the page.
	pagetotal += vlist[vlist_index].GetLength();
	pagestretch += vlist[vlist_index].GetStretch();
	pageshrink += vlist[vlist_index].GetShrink();

	# Advance to vertical list index to the next
	# free position.
	vlist_index++;

	# Have we generated a new valid breakpoint?
	if(vlist_index > 2)			# can't break one item
		{
		if(type == (OType)glue || type == (OType)penalty)
			{					# break at glue and penalties
			int thispenalty;

			# Collect the penalty value as we pass.
			if(type == (OType)penalty)
				thispenalty = ((Penalty*)ptr)->GetValue();
			else
				thispenalty = 0;

			# How much must the page size be altered to
			# fit the pagegoal?
			SP change;
			change = pagegoal - pagetotal;

			# What glue change ratio is required?
			double ratio;
			if(change > 0)		# stretch
				{
				if(pagestretch.GetInfinity() > 0)
					ratio = 0;
				else
					ratio = (double)change.GetValue() / (double)pagestretch.GetValue();
				}
			else if(change < 0)	# shrink
				{
				ratio = (double)(change.GetValue() * -1) / (double)pageshrink.GetValue();
				}
			else					# no change
				{
				ratio = 0.0;
				}

			# Turn this into a badness value.
 			int badness;
			if(change < 0 && (change + pageshrink) < 0)
				{					# Overfull hboxes
				badness = 10001;	# are infinitly bad.
				}
			else
				{
				if(ratio > 10.0)	# prevent aritmatic overflows
					{
					badness = 10000;
					}
				else
					{
					badness = (int)(ratio * ratio * ratio * 100.0);
					if(badness > 10000)
						badness = 10000;
					}
				}

			# Turn the badness, the penalty, and insertpenalties
			# into a cost value.
			int cost;
			if(badness < 10001 && thispenalty <= -10000 && insertpenalties < 10000)
				cost = thispenalty;
			else if(badness < 10000 && -10000 < thispenalty && thispenalty < 10000 && insertpenalties < 10000)
				cost = badness + thispenalty + insertpenalties;
			else if(badness==10000 && -10000 < thispenalty && thispenalty < 10000 && insertpenalties < 10000)
				cost = 100000;
			else if( (badness==10001 || insertpenalties >= 10000) && thispenalty < 10000 )
				cost = 100001;
			else
				throw("Typesetter::contribute_vertical(): cost algarithm failed\n");

			# trace output
			if(tracingpages)
				{
				cerr << "% t=" << pagetotal << " plus " << pagestretch
					<< " minus " << pageshrink << " g=" << pagegoal
					<< " b=" << badness << " p=" << thispenalty
					<< " c=" << cost;
				}

			# Best so far?
			if(cost <= best_cost)
				{
				best_so_far			= vlist_index;
				best_cost			= cost;
				best_change			= change;
				best_pagestretch	= pagestretch;
				best_pageshrink		= pageshrink;

				if(tracingpages)
					cerr << "#";
				}

			# If we have been generating a trace line,
			# end that line.
			if(tracingpages)
				cerr << "\n";

			# Is this one infinitly costly?  If it is,
			# we must break at the best one.  We must also
			# break at the best point so far if a penalty
			# of -10000 or less forces a page break.
			if(cost == 100001 || thispenalty <= -10000)
				{
				# Allocate an array of objects to hold the
				# entire broken off page.
				Object *members = new Object[best_so_far];

				# Break off the page by copying its members
				# into the members list we are creating,
				# totaling up the vbox's width, height, and
				# depth and setting glue as we go.
				int x;
				ISP w=0;
				ISP h=0;
				ISP d=0;
				int iorder;			# max infinity order in stretch or shrink

				if(best_change > 0)
					iorder = best_pagestretch.GetInfinity();
				else
					iorder = best_pageshrink.GetInfinity();

				for(x=0; x < best_so_far; x++)
					{
					members[x] = vlist[x];
					if(members[x].GetType() == (OType)glue)
						{
						Glue *this_glue = (Glue*)(members[x].GetPtr());
						ISP newwidth = this_glue->GetWidth();
						#ifdef DEBUG_PAGE_GLUESET
						cerr << "page glue: width=" << newwidth << '\n';
						#endif
						if(best_change > 0)			# glue stretch
							{
							if( this_glue->GetStretch().GetInfinity() >= iorder )
								{
								#ifdef USE_FP
								newwidth += (int)( (double)(this_glue->GetStretch().GetValue()) / (double)(best_pagestretch.GetValue()) * (double)(best_change.GetValue()) + 0.5);
								#else
								newwidth += ( (best_change.GetValue() >> 8) * (this_glue->GetStretch().GetValue() >> 8)
									/ (best_pagestretch.GetValue() >> 16) );
								#endif
								}
							}
						else if(best_change < 0)	# glue shrink
							{
							if(this_glue->GetShrink().GetInfinity() >= iorder)
								{
								#ifdef USE_FP
								newwidth += (int)( (double)(this_glue->GetShrink().GetValue()) / (double)(best_pageshrink.GetValue()) * (double)(best_change.GetValue()) + 0.5);
								#else
								newwidth += ( (best_change.GetValue() >> 8) * (this_glue->GetShrink().GetValue() >> 8)
										/ (best_pageshrink.GetValue() >> 16) );
								#endif
								}
							}
						#ifdef DEBUG_PAGE_GLUESET
						cerr << "newwidth=" << newwidth << '\n';
						#endif
						this_glue->SetWidth(newwidth);
						}
					}

				# Make that new list into a SuperBox.
				SuperBox *obox = new SuperBox(w, h, d, members, best_so_far);

				# Make the SuperBox a vbox object.
				Object *obj = new Object;
				obj->Set( (OType)vbox, (void*)obox );

				# Send it to the output routine.  The default
				# Typesetter::Output() simply calls PostScript::ShipOut().
				# If the default routine is not overridden, calling
				# Output() will result in the immediate generation
				# of PostScript.
				#
				# Since the output routine may keep the box as long
				# as it likes, perhaps waiting to combine it with
				# later output, it must free the objects itself.
				Output(obj);

				# Clear the font usage flags.
				for(x=0; x < fonts_index; x++)
					fonts[x]->ClearPageFlags();

				# Re-contribute the remaining stuff
				# to the current page.
				int stop = vlist_index;
				vlist_index = 0;
				for(x = best_so_far; x < stop; x++)
					contribute_vertical(vlist[x].GetType(), vlist[x].GetPtr());
				} # end of if must use best breakpoint now

			} # end of if valid breakpoint

		} # end of if there are enough members for a breakpoint

	} # end of Typesetter::contribute_vertical()

#
# Contribute material to the current horizontal list.
# If we are in a vertical mode, enter horizontal mode first.
#
# private:
void Typesetter::contribute_horizontal(OType type, void *ptr)
	{
	#ifdef DEBUG_LISTS
	cerr << "Typesetter::contribute_horizontal(type=" << type << ", ptr=" << ptr << ")\n";
	Object xd(type, ptr);
	xd.debug(1);
	#endif

	# If we are not now in a horizontal mode,
	# get into one.  We use push_mode() because
	# we want to remember what mode we were in.
	if(mode == (Mode)vertical || mode == (Mode)restricted_vertical)
		{
	    cerr << "Typesetter::contribute_horizontal(): switching to horizontal mode\n";
		push_mode( (Mode)horizontal );
		}

	if(hlist_index == hlist_size)
		throw("Typesetter::contribute_horizontal(): horizontal list has overflowed");

	# Now, we can actually add this to the list.
	#ifdef DEBUG_LISTS
	cerr << "Typesetter::contribute_horizontal(): hlist_index is " << hlist_index << "\n";
	#endif
	hlist[hlist_index++].Set(type, ptr);
	} # end of Typesetter::contribute_horizontal()

#
# Set a string in type.
# This is simply an example routine which calls
# AddCharacter() and HSkip().
#
void Typesetter::AddString(const char *s)
	{
	for( ; *s; s++)
		{
		if(*s == ' ')					# spaces become glue
			{
			if(s[1] == ' ')				# double space becomes
				{
				HSkip(xspaceskip);		# sentence end glue
				s++;
				}
			else						# single space becomes
				{
				HSkip(spaceskip);		# inter-word glue
				}
			}
		else							# Characters just get
			{							# added with
			AddCharacter(*s);			# the routine for that purpose.
			}
		}
	} # end of Typesetter::AddString()

#
# Insert the interline glue which should go before an object
# of the specified height.  This is called by paragraph_to_vlist().
#
# I think prevdepth is the depth (below its baseline) of the last
# object contributed to the vertical list.
#
# If prevdepth is -10000, interline glue is suppressed.
# ``height'' and ``depth'' are the height and depth of the box we are
# about to contribute to the vertical list.
#
# private:
void Typesetter::insert_interline_glue(SP& height, SP& depth)
	{
	if(prevdepth != -10000)
		{
		SP distance;
		SP stretch;
		SP shrink;

		distance = baselineskip.GetWidth();
		stretch = baselineskip.GetStretch();
		shrink = baselineskip.GetShrink();

		distance -= prevdepth;
		distance -= height;

		if( distance < lineskiplimit )
			contribute_vertical( (OType)glue, (void*)new Glue(lineskip) );
		else
			contribute_vertical( (OType)glue, (void*)new Glue(distance,stretch,shrink) );
		}

	# Update prevdepth
	prevdepth = depth.GetValue();
	} # end of Typesetter::insert_interline_glue()

#
# Find breakpoints.
# An internal routine used by EndParagraph().
#
# It returns a value in bpcount.
#
# private:
int Typesetter::find_breakpoints(int current_tolerance)
	{
	int hi;					# horizontal index
	int x;					# generaly utility index
	int thispenalty;		# the penalty at current breakpoint
	int feasibles_size = 100;
	Breakpoint feasibles[feasibles_size];
	int feasibles_count;
	int text_printed = false;

	cerr << "Typesetter::find_breakpoints()\n";

	bplist[0].Set(hsize, 0, 0);			# first breakpoint at begining of paragraph
	bpcount = 1;						# that makes a total of one breakpoint

	feasibles[0] = bplist[0];			# our first feasible breakpoint will start its hbox here
	feasibles_count = 1;

	# Add stuff to all the currently potentially feasible breakpoints
	# until they become feasible or they overflow.
	Glue *this_glue;
	SP this_width;
	Discretionary *this_discretionary;
	SP this_width_pre;
	SP this_width_post;
	SP this_width_no;
	for(hi=0; hi < hlist_index; hi++)
		{
		#ifdef DEBUG_LISTS
		cerr << "hlist item " << hi << "\n";
		#endif

		thispenalty = 0;					# clear any old penalty

		# If the current horizontal list item is not a legal breakpoint,
		# add its size to all potentially feasible breakpoints and "continue",
		# otherwise, drop down to the feasibility testing code.
		switch( hlist[hi].GetType() )
			{
			case (OType)empty:				# problem!
				throw("Typesetter::find_breakpoints(): empty hlist[] item\n");

			case (OType)character:			# Breaks are not allowed at characters
				if(tracingparagraphs)
					{
		 			cerr << (char)( ((CBox*)(hlist[hi].GetPtr())) -> GetCharCode() );
		 			text_printed = true;
		 			}
				# fall thru

		 	case (OType)hbox: 				# at hboxes
		 	case (OType)vbox:				# at vboxes
		 	case (OType)vrule:				# or at vrules
				this_width = ((Box *)hlist[hi].GetPtr()) -> GetWidth();
				for(x=0; x < feasibles_count; x++)
					if( hi >= feasibles[x].GetHboxStart() )
						feasibles[x].AddBox( this_width );
				continue;					# so don't reach end of loop.

			case (OType)glue:				# Glue
				if(tracingparagraphs)		# Possibly put in trace output
					{
					cerr << " ";
					text_printed = true;
					}
				break;

			case (OType)disbreak:
				this_discretionary = (Discretionary*)hlist[hi].GetPtr();
				this_width_pre = this_discretionary -> GetPre()->GetWidth();
				this_width_post = this_discretionary -> GetPost()->GetWidth();
				this_width_no = this_discretionary -> GetNo()->GetWidth();
				if(tracingparagraphs)
					cerr << "-";
				for(x=0; x < feasibles_count; x++)
					{
					if( hi >= feasibles[x].GetHboxStart() )		# is this condition necessary?
						{
						feasibles[x].AddBox(this_width_pre);
						feasibles[x].SetRightDisBreak(true);	# it ends with a disbreak
						}
					}

				# If the pre-break text is non-empty, set the penalty
				# to "hyphenpenalty", otherwise, set it to
				# "exhyphenpenalty".
				if(this_discretionary -> GetPre()->GetNumMembers() > 0)
					thispenalty = hyphenpenalty;
				else
					thispenalty = exhyphenpenalty;

				break;

			case (OType)penalty:			# penalty gets noted, is a breakpoint
				thispenalty = ((Penalty*)hlist[hi].GetPtr()) -> GetValue();
				break;

			case (OType)kern:				# A kern is a breakpoint,
				if((hi + 1) < hlist_index && hlist[hi+1].GetType() == (OType)glue)
					{						# if it is followed by glue,
					break;
					}
				else						# otherwise,
					{						# it is just another box.
					this_width = ((Kern *)hlist[hi].GetPtr()) -> GetWidth();
					for(x=0; x < feasibles_count; x++)
						feasibles[x].AddBox(this_width);
					continue;
					}

			case (OType)hrule:
				throw("find_breakpoints(): hrules are illegal in horizontal lists");
			}

		#--------------------------------------------------------------
		# If we get here, (note the use of "continue" above) we have
		# a potential breakpoint.  Now we must decide if it is feasible.
		#--------------------------------------------------------------
		int lowest_demerits = -1;
		int lowest_so_far = -1;
		for(x=0; x < feasibles_count; x++)
			{
			int badness;
			int demerits;

			badness = feasibles[x].Badness();

			#
			# If the badness exceeds the current tolerance, forget it
			# unless a highly negative penalty forces a breakpoint here.
			# Even if the badness is not greater than the tolerance, we
			# can not use this breakpoint if the penalty is 10000 or
			# higher.
			#
			# This if() is true if the breakpoint passes all of
			# those hurdles.
			#
			if(hi >= feasibles[x].GetHboxStart()
					&& ( (badness <= current_tolerance || thispenalty <= -10000)
						|| (feasibles_count==1 && feasibles[x].Overfull()) )
					&& thispenalty < 10000)
				{
				# Compute the basic demerits.
				demerits = feasibles[x].Demerits(badness, linepenalty, thispenalty);

				# Add demerits if this line and the preceeding one
				# are visually incompatible.  The first if() says,
				# "only bother if this is not the first line and if
				# this line and the previous one are not overfull".
				int prev = feasibles[x].GetVia();

				#ifdef DEBUG_LINEBREAK
				cerr << "bpcount = " << bpcount << ", feasibles_count = " << feasibles_count << ", x = " << x << ", prev = " << prev << "\n";
				#endif
#				if( prev > 0 && ! feasibles[x].Overfull() && ! feasibles[prev].Overfull() )
				if( prev > 0 && ! feasibles[x].Overfull() && ! bplist[prev].Overfull() )
					{
					# The amount the line represented by this breakpoint
					# would have to change.
					SP change = feasibles[x].GetBPhsize() - feasibles[x].GetBoxes() - feasibles[x].GetGlue();

					# The amount the line represented by the parent
					# breakpoint would have to change.
					SP prevchange = bplist[prev].GetBPhsize() - bplist[prev].GetBoxes() - bplist[prev].GetGlue();

					# The badness of the line represented by the
					# parent breakpoint.
					int prevbadness = bplist[prev].Badness();

					# A line which has shrunk so much that its
					# badness exceeds 100 will have been reduced to
					# a negative width, which would be distinctly odd.
					if( badness > 100 && change < 0 )
						{
						cerr << "Typesetter::paragraph_to_vlist(): can't happen: badness="
							<< badness << ", change=" << change << "\n";
						throw("internal error");
						}
					if(prevbadness > 100 && prevchange < 0)
						{
						cerr << "Typesetter::paragraph_to_vlist(): can't happen: prevbadness="
							<< prevbadness << ", prevchange=" << prevchange << "\n";
						throw("internal error");
						}

					# If previous line was tight and this line
					# is loose or very loose, add visual
					# incompatibility demerits.
					if( ( prevchange < 0 && prevbadness >= 13
							&& change > 0 && badness >= 13 )
						# Or if previous line was decent
						# and this line is very loose,
						|| ( prevbadness <= 12 && badness >= 100 )
						# Or if previous line was loose and this
						# line is tight,
						|| ( prevchange > 0 && prevbadness >= 13
							&& change < 0 && badness >= 13 )
						# Or if previous line was very loose and
						# this line is decent or tight
						|| ( prevchange > 0 && prevbadness >= 100
							&& (badness <= 12 || change < 0) )
						)
						{
						demerits += adjdemerits;
						#ifdef DEBUG_LINEBREAK
						cerr << "* visual incompatibility!\n";
						#endif
						}
					}

				# Add demerits if this line ends with a broken discretionary
				# with non-empty pre-break text and the previous line
				# does too.
#				if( feasibles[x].GetVia() > 0 && feasibles[x].GetRightDisBreak()
#						&& feasibles[ feasibles[x].GetVia() ].GetRightDisBreak() )
				if( feasibles[x].GetVia() > 0 && feasibles[x].GetRightDisBreak()
						&& bplist[ feasibles[x].GetVia() ].GetRightDisBreak() )
					{
					demerits += doublehyphendemerits;
					#ifdef DEBUG_LINEBREAK
					cerr << "* consecutive hyphen lines!\n";
					#endif
					}

				# Add demerits if this is the last line of the
				# paragraph and the previous line ended with
				# a broken discretionary with non-empty pre-
				# break text.
#				if( hi == (hlist_index - 1)
#						&& feasibles[ feasibles[x].GetVia() ].GetRightDisBreak() )
				if( hi == (hlist_index - 1)
						&& bplist[ feasibles[x].GetVia() ].GetRightDisBreak() )
					{
					demerits += finalhyphendemerits;
					#ifdef DEBUG_LINEBREAK
					cerr << "* final hyphen demerits apply\n";
					#endif
					}

				# Store the some of these demerits and the total of the
				# demerits of all ancestor breakpoints.
				feasibles[x].SetTotalDemerits( demerits + (bplist[feasibles[x].GetVia()].GetTotalDemerits()) );

				if(hlist[hi].GetType() != (OType)disbreak)		# If it is glue or penalty,
					feasibles[x].SetLocation(hi);				# we break just before it
				else											# for a discretionary,
					feasibles[x].SetLocation(hi+1);				# we break just after it.

				# TeX style breakpoint tracing
				if(tracingparagraphs)
					{
					if(text_printed)			# If we printed some of
						{						# the list contents,
						cerr << "\n";			# start a new line.
						text_printed = false;
						}
					cerr << "@";
					switch(hlist[hi].GetType())
						{
						case (OType)glue:
							cerr << "glue";
							break;
						case (OType)kern:
							cerr << "kern";
							break;
						case (OType)penalty:
							cerr << "penalty";
							break;
						case (OType)disbreak:
							cerr << "\\discretionary";
							break;
						default:
							cerr << "illegal";
							break;
						}
					cerr << " via @@" << feasibles[x].GetVia() << " b=" << badness
						<< " p=" << thispenalty << " d=" << demerits << "\n";
					} # end of if(tracingparagraphs)

				# If this is the first breakpoint, or this one
				# has the lowest number of demerits so far,
				# record it for future reference.
				if(lowest_demerits == -1 || demerits <= lowest_demerits)
					{
					lowest_demerits = demerits;
					lowest_so_far = x;
					}

				} # end of if feasible

			} # end of for loop which computes all the badnesses

		# If we found any feasible breakpoints or an overfull hbox
		# which must be sent out anyway, send out the one with the
		# lowest number of demerits.
		if(lowest_demerits != -1)
			{
			if(bpcount >= bplist_size)
				throw("find_breakpoints(): breakpoint list overflow");

			bplist[bpcount] = feasibles[lowest_so_far];

			# If this breakpoint was forced by a penalty of -10000,
			# we can throw away all but the potential breakpoint we
			# are about to start.
			if(thispenalty == -10000)
				{
				#ifdef DEBUG_LINEBREAK
				cerr << "*** Penalty -10000 cancels feasible breakpoints\n";
				#endif
				feasibles_count = 0;
				}

			# Now, we must emmit the trace record which
			# shows which breakpoint we selected.
			if(tracingparagraphs)
				{
				cerr << "@@" << bpcount << ": offset " << hi
					<< " t=" << bplist[bpcount].GetTotalDemerits() << "\n";
				}

			# Find where a breakpoint which immedialty followed
			# would begin.  To do this, we must discard anything
			# discardable.
			for(x = hi; x < hlist_index; x++)
				{
				OType type;
				type = hlist[x].GetType();
				if(type != (OType)glue && type != (OType)kern && type != (OType)penalty)
					break;
				}

			# Start a new possibly feasible breakpoint structure
			# which starts where the breakpoint we just
			# discovered leaves off
			if(feasibles_count >= feasibles_size)
				throw("find_breakpoints(): feasibles list overflow");

			feasibles[feasibles_count].Set(hsize, bpcount, x);

			bpcount++;
			feasibles_count++;
			} # end of if(lowest_demerits!=-1)

		# If there is more than one potentially feasible breakpoint in the list,
		# remove formerly potentially feasible breakpoints which have overflowed.
		if(feasibles_count > 1)
			{
			int y;
			for(x=0; x < feasibles_count; x++)
				{
				if( feasibles[x].Overfull() )
					{
					feasibles_count--;
					for(y=x; y < feasibles_count; y++)
						feasibles[y] = feasibles[y+1];
					}
				}
			}

		# Add glue and kerns which we held back until the
		# breakpoint was evaluated.
		switch( hlist[hi].GetType() )
			{
			case (OType)glue:				# Glue
				this_glue=(Glue*)hlist[hi].GetPtr();
				for(x=0; x < feasibles_count; x++)
					if( hi >= feasibles[x].GetHboxStart() )
						feasibles[x].AddGlue( this_glue );
				break;

			case (OType)kern:				# A kern is a breakpoint,
				this_width = ((Kern *)hlist[hi].GetPtr()) -> GetWidth();
				for(x=0; x < feasibles_count; x++)
					feasibles[x].AddBox(this_width);
				break;

			case (OType)disbreak:
				for(x=0; x < feasibles_count; x++)
					{
					if( hi == feasibles[x].GetHboxStart() )
						{			# if at start, include post-break text
						feasibles[x].AddBox(this_width_post);
						feasibles[x].SetLeftDisBreak(true);
						}
					else			# revoke the break
						{
						feasibles[x].AddBox(this_width_pre * -1);
						feasibles[x].AddBox(this_width_no);
						feasibles[x].SetRightDisBreak(false);
						}
					}
				break;

			default:
				break;
			}
		} # end of loop which addes items to potentially feasible breakpoints

	# Mark the path back `golden'.
	# Notice that is fails to mark breakpoint 0 golden.
	# This deficiency doesn't really matter.
	for(x=(bpcount-1); x > 0; )
		{
		bplist[x].SetGolden(true);
		x = bplist[x].GetVia();
		}

	# If any of the golden breakpoints are at the
	# end of overfull hboxes, return -1.
	for(x=0; x < bpcount; x++)
		{
		if( bplist[x].IsGolden() && bplist[x].Overfull() )
			return -1;
		}

	return 0;
	} # end of Typesetter::find_breakpoints()

#
# Using the breakpoints found by the above function,
# break the paragraph into hboxes and append those
# hboxes to the vertical list.
#
# An internal routine used by EndParagraph().
#
# private:
void Typesetter::paragraph_to_vlist(void)
	{
	int x, y;
	int start, length;
	Object *members;
	SP twidth, theight, tdepth;
	SP sanity;
	SP change;							# amount glue must stretch (neg means shrink)
	int iorder;							# order of infinity to use
	int lastclaimed = 0;

	cerr << "Typesetter::paragraph_to_vlist()\n";

	for(x=1; x < bpcount; x++)			# Go thru the whole list
		{								# of breakpoints.
		if( ! bplist[x].IsGolden() )	# Skip those that are not `golden'.
			continue;

		#ifdef DEBUG_GLUESET
		cerr << "\nbreakpoint " << x << "\n";
		#endif

		# We have a breakpoint, determine where in the
		# horizontal list its hbox starts and how far the
		# hbox continues.
		start = bplist[x].GetHboxStart();
		length = bplist[x].GetLocation() - start;

		# Allocate memory to hold a new copy of the members.
		members = new Object[length];

		# Clear total width, height, and depth.
		twidth = theight = tdepth = 0;
		sanity = 0;

		# Determine how much the glue must change.
		change = bplist[x].GetBPhsize() - bplist[x].GetBoxes() - bplist[x].GetGlue();

		if(change > 0)			# If shretch,
			{					# determine the maximum infinity in glue stretch.
			iorder = bplist[x].GetMaxStretch().GetInfinity();
			}
		else					# If shrink, do same, but also
			{					# cut shrink off at limit.
			iorder = bplist[x].GetMaxShrink().GetInfinity();
			if( change < (bplist[x].GetMaxShrink() * -1) )
				change = bplist[x].GetMaxShrink() * -1;
			}

		#ifdef DEBUG_GLUESET
		cerr << "hsize=" << bplist[x].GetBPhsize() << ", boxes=" << bplist[x].GetBoxes()
			<< ", glue=" << bplist[x].GetGlue()
			<< ", change=" << change
			<< ", maxstretch=" << bplist[x].GetMaxStretch()
			<< ", maxshrink=" << bplist[x].GetMaxShrink() << "\n";
		#endif

		# Free memory which would otherwise leak.
		# This memory is occupied by horizontal glue items and such which
		# fall between lines and are discarded from the horizontal list.
		for(y = lastclaimed + 1; y < start; y++)
			{
			#ifdef DEBUG_LEAKS
			cerr << "Typesetter::paragraph_to_vlist(): freeing leak hlist[" << y << "]\n";
			#endif
			hlist[y].Clear();
			}

		# Iterate thru the hlist members which will go into
		# the next hbox, adding them to the hbox as we go.
		Discretionary *this_disbreak;
		Box *this_box;
		for(y=0; y < length; y++)
			{
			lastclaimed = start + y;
			members[y] = hlist[lastclaimed];	# Copy the member into the hbox

			#ifdef DEBUG_LEAKS
			cerr << "Typesetter::paragraph_to_vlist(): using hlist[" << lastclaimed << "]\n";
			#endif

			switch(members[y].GetType())	# act according to the type of the member
				{
				case (OType)disbreak:		# Discretionary break
					this_disbreak = (Discretionary*)members[y].GetPtr();
					if(y == 0 && bplist[x].GetLeftDisBreak())
						{
						#ifdef DEBUG_LISTS
						cerr << "Claiming discretionary post\n";
						#endif
						this_box = this_disbreak->ClaimPost();
						members[y].Clear();
						}
					else if( (length-y)==1 && bplist[x].GetRightDisBreak() )
						{
						#ifdef DEBUG_LISTS
						cerr << "Claiming discretionary pre\n";
						#endif
						this_box = this_disbreak->ClaimPre();
						# don't clear until post is claimed
						}
					else
						{
						#ifdef DEBUG_LISTS
						cerr << "Claiming discretionary nobreak\n";
						#endif
						this_box = this_disbreak->ClaimNo();
						members[y].Clear();
						}
					members[y].Set( (OType)hbox, (void*)this_box );
					# drop thru

				case (OType)character:		# Various boxes,
				case (OType)hbox:			# just keep dimension
				case (OType)vbox:			# records.
					this_box=(Box*)members[y].GetPtr();
					twidth += this_box->GetWidth();
					sanity += this_box->GetWidth();
					if(this_box->GetHeight() > theight) theight=this_box->GetHeight();
					if(this_box->GetDepth() > tdepth) tdepth=this_box->GetDepth();
					break;

				case (OType)kern:			# Kern, just keep dimension records.
					twidth += ((Kern*)(members[y].GetPtr()))->GetWidth();
					break;

				case (OType)glue:			# Set glue.
					{
					Glue *g;
					SP newsize;
					g=(Glue*)members[y].GetPtr();

					# Start new size out as the old size.
					newsize = g->GetWidth();

					#ifdef DEBUG_GLUESET
					cerr << "glue item " << y << " width=" << g->GetWidth()
						<< ", stretch=" << g->GetStretch()
						<< ", shrink=" << g->GetShrink() << "\n";
					#endif

					if(change > 0)			# stretch glue to fit
						{
						if(g->GetStretch().GetInfinity() >= iorder)	# if this blob not superseeded,
							{
							#ifdef USE_FP
							newsize += (int)( (double)(g->GetStretch().GetValue()) / (double)(bplist[x].GetMaxStretch().GetValue()) * (double)(change.GetValue()) + 0.5 );
							#else
							newsize += ( (g->GetStretch().GetValue() >> 6) * (change.GetValue() >> 6)
									 / (bplist[x].GetMaxStretch().GetValue() >> 12) );
							#endif
							}
						}
					else					# shrink glue to fit
						{
						if(g->GetShrink().GetInfinity() >= iorder)
							{
							#ifdef USE_FP
							newsize += (int)( (double)(g->GetShrink().GetValue()) / (double)(bplist[x].GetMaxShrink().GetValue()) * (double)(change.GetValue()) + 0.5 );
							#else
							newsize += ( (change.GetValue() >> 6) * (g->GetShrink().GetValue() >> 6)
							 		/ (bplist[x].GetMaxShrink().GetValue() >> 12) );
							#endif
							}
						}

					#ifdef DEBUG_GLUESET
					cerr << "new width=" << newsize << "\n";
					#endif

					# Set the new glue dimensions.
					g->SetWidth(newsize);

					# Add to the total width of the enclosing hbox.
					twidth += newsize;
					}
					break;

				default:					# Don't do anything for anything else.
					# no code
					break;
				}
			}

		#ifdef DEBUG_GLUESET
		cerr << "final width=" << twidth << ", glueset error=" << twidth - bplist[x].GetBPhsize() << "\n";
		#endif

		# Do the sanity check.
		if( bplist[x].GetBoxes() != sanity )
			{
			cerr << "paragraph_to_vlist(): sanity check failed,\n";
			cerr << "bplist[x].GetBoxes() = " << bplist[x].GetBoxes()
				<< "sanity = " << sanity << '\n';
			exit(1);
			}

		# Posibly add interline glue.
		insert_interline_glue(theight,tdepth);

		# Add the hbox to the vertical list.
		contribute_vertical( (OType)hbox, new SuperBox(twidth,theight,tdepth,members,length) );
		} # end of loop which steps thru the good breakpoints

	for(y=lastclaimed+1; y < hlist_index; y++)
		{
		#ifdef DEBUG_LEAKS
		cerr << "freeing trailing leak hlist[" << y << "]\n";
		#endif
		hlist[y].Clear();
		}

	} # end of Typesetter::paragraph_to_vlist()

#
# Turn the current horizontal list into a series of vboxes on
# the current vertical list.  After that is done, go to
# vertical mode.
#
# This function does nothing if the horizontal list is empty.
#
void Typesetter::EndParagraph(void)
	{
	cerr << "Typesetter::EndParagraph()\n";

	if(mode != (Mode)horizontal)
		{
		cerr << "warning: EndParagraph() does nothing except in horizontal mode\n";
		return;
		}

	if(hlist_index)					# if there is something to dump
		{
		while(hlist_index)			# delete trailing glue
			{
			if(hlist[hlist_index-1].GetType() == (OType)glue)
				hlist_index--;
			else
				break;
			}

		AddPenalty(10000);			# prohibit line break

		HSkip(parfillskip);			# add finishing glue

		AddPenalty(-10000);			# force a line break

		if(find_breakpoints(tolerance))
			cerr << "Warning: overfull hbox(s)\n";

		paragraph_to_vlist();		# make hboxes and put on vertical list
		}

	hlist_index = 0;
	pop_mode();						# return to previous mode
	} # end of Typesetter::EndParagraph()

# Add a kern to the current horizontal list
void Typesetter::AddHKern(const SP width)
	{
	contribute_horizontal( (OType)kern, (void*)new Kern(width,true) );
	} # end of Typesetter::AddHKern()

# Add a kern to the current horizontal list
void Typesetter::AddVKern(const SP width)
	{
	switch_to_vertical();
	contribute_vertical( (OType)kern, (void*)new Kern(width,true) );
	} # end of Typesetter::AddVKern()

# Add glue to the current horizontal list
void Typesetter::HSkip(const Glue& glue_obj)
	{
	Glue *newglue = new Glue = glue_obj;

	contribute_horizontal( (OType)glue, (void*)newglue );
	} # end of Typesetter::HSkip()

void Typesetter::HSkip(const SP& width, const SP& stretch, const SP& shrink)
	{
	contribute_horizontal( (OType)glue, (void*)new Glue(width,stretch,shrink) );
	} # end of Typesetter::HSkip()

# Add glue to the current vertical list
void Typesetter::VSkip(Glue& glue_obj)
	{
	switch_to_vertical();
	contribute_vertical( (OType)glue, (void*)(new Glue=glue_obj) );
	} # end of Typesetter::VSkip()

void Typesetter::VSkip(const SP& width, const SP& stretch, const SP& shrink)
	{
	switch_to_vertical();
	contribute_vertical( (OType)glue, (void*)new Glue(width,stretch,shrink) );
	} # end of Typesetter::VSkip()

# Add a penalty to the current horizontal
# or vertical list, depending on the mode.
void Typesetter::AddPenalty(const int value)
	{
	switch(mode)
		{
		case (Mode)horizontal:
		case (Mode)restricted_horizontal:
			contribute_horizontal( (OType)penalty, (void*)new Penalty(value) );
			break;
		case (Mode)vertical:
		case (Mode)restricted_vertical:
			contribute_vertical( (OType)penalty, (void*)new Penalty(value) );
			break;
		}
	} # end of Typesetter::AddPenalty()

#
# Begin a group.
# Save all the TeX parameters.
#
void Typesetter::BeginGroup(void)
	{

	} # end of Typesetter::BeginGroup()

#
# End a group.
# Restore the previosly saved TeX parameters.
#
void Typesetter::EndGroup(void)
	{

	} # end of Typesetter::EndGroup()

#
# Save the current mode.
#
# private:
void Typesetter::push_mode(Mode newmode)
	{
	if(stackptr_mode == STACKSIZE_MODE)
		throw("push_mode(): Mode stack overflow");

	stack_mode[stackptr_mode].mode = mode;
	mode = newmode;

	switch(newmode)
		{
		case (Mode)horizontal:
		case (Mode)restricted_horizontal:
			stack_mode[stackptr_mode].list = hlist;
			stack_mode[stackptr_mode].list_size = hlist_size;
			stack_mode[stackptr_mode].list_index = hlist_index;
			break;
		case (Mode)vertical:
		case (Mode)restricted_vertical:
			stack_mode[stackptr_mode].list = vlist;
			stack_mode[stackptr_mode].list_size = vlist_size;
			stack_mode[stackptr_mode].list_index = vlist_index;
			break;
		}

	switch(newmode)
		{
		case (Mode)horizontal:
			hlist_size = newhlist;
			hlist = new Object[hlist_size];
			hlist_index = 0;
			break;
		case (Mode)restricted_horizontal:
			hlist_size = newrhlist;
			hlist = new Object[hlist_size];
			hlist_index = 0;
			break;
		case (Mode)vertical:
			throw("push_mode(): Vertical mode can not be enclosed");
		case (Mode)restricted_vertical:
			vlist_size = newrvlist;
			vlist = new Object[vlist_size];
			vlist_index = 0;
			break;
		}

	stackptr_mode++;
	} # end of Typesetter::push_mode()

#
# Restore the last saved mode.
# It is very important to set hlist_index or vlist_index
# to zero if you do not want the stuff on the list to be
# deleted by this function.
#
# private:
void Typesetter::pop_mode(void)
	{
	int x;

	if(stackptr_mode == 0)
		throw("pop_mode(): Mode stack underflow");

	stackptr_mode--;

	switch(mode)
		{
		case (Mode)vertical:
			throw("pop_mode(): it is not permissable to pop vertical mode");
		case (Mode)restricted_vertical:
			# Delete all of the objects remaining:
			for(x=0; x < vlist_index; x++)
				vlist[x].Clear();

			# Delete storage for the list array itself:
			delete vlist;

			# Reset the pointer to the previous vlist:
			vlist = stack_mode[stackptr_mode].list;
			vlist_size = stack_mode[stackptr_mode].list_size;
			vlist_index = stack_mode[stackptr_mode].list_index;
			break;
		case (Mode)horizontal:
		case (Mode)restricted_horizontal:
			# Throw away the contents of all of the objects
			# on the restricted horizontal list:
			for(x=0; x < hlist_index; x++)
				hlist[x].Clear();

			# Delete the storage for the restricted horizontal list:
			delete hlist;

			# Go back to the previous horizontal list:
			hlist = stack_mode[stackptr_mode].list;
			hlist_size = stack_mode[stackptr_mode].list_size;
			hlist_index = stack_mode[stackptr_mode].list_index;
			break;
		}

	# Actually restore the mode.
	mode = stack_mode[stackptr_mode].mode;
	} # end of Typesetter::pop_mode()

#
# Begin an hbox.
#
void Typesetter::BeginHBox(void)
	{
	push_mode( (Mode)restricted_horizontal );
	BeginGroup();
	} # end of Typsetter::BeginHBox()

#
# End an hbox.
#
void Typesetter::EndHBox(void)
	{
	# missing code

	EndGroup();
	pop_mode();
	} # end of Typesetter::EndHBox()

#
# Begin a vbox.
#
void Typesetter::BeginVBox(void)
	{
	push_mode( (Mode)restricted_vertical );
	BeginGroup();
	} # end of Typesetter::BeginVBox()

#
# End a vbox.
#
void Typesetter::EndVBox(void)
	{
	# missing code

	EndGroup();
	pop_mode();
	} # end of Typesetter::EndVBox()

#
# Discretionary break routines.
#
void Typesetter::DisStart(void)
	{
	if(dis_stage != 0)
		throw("DisStart(): unfinished discretionary already exists");

	push_mode( (Mode)restricted_horizontal );
	dis_stage = 1;
	} # end of Typesetter::DisStart()

void Typesetter::DisPost(void)
	{
	if(dis_stage != 1)
		throw("DisPost(): call order is DisStart(), DisPost(), DisNo(), DisEnd()");

	# Put the pre-break text into an hbox.
	Object *members = new Object[hlist_index];
	int x;
	SP w,h,d;
	w = h = d = PT(0);
	for(x=0; x < hlist_index; x++)
		{
		members[x] = hlist[x];
		w += hlist[x].GetWidth();
		if(hlist[x].GetHeight() > h) h = hlist[x].GetHeight();
		if(hlist[x].GetDepth() > d) d = hlist[x].GetDepth();
		}
	dis_pre = new SuperBox(w, h, d, members, hlist_index);

	hlist_index = 0;	# clear our stuff off the horizontal list.
	dis_stage = 2;
	} # end of Typesetter::DisPost()

void Typesetter::DisNo(void)
	{
	if(dis_stage != 2)
		throw("DisNo(): call order is DisStart(), DisPost(), DisNo(), DisEnd()");

	# Put the post-break text into an hbox.
	Object *members = new Object[hlist_index];
	int x;
	SP w,h,d;
	w = h = d = PT(0);
	for(x=0; x < hlist_index; x++)
		{
		members[x] = hlist[x];
		w += hlist[x].GetWidth();
		if(hlist[x].GetHeight() > h) h = hlist[x].GetHeight();
		if(hlist[x].GetDepth() > d) d = hlist[x].GetDepth();
		}
	dis_post = new SuperBox(w, h, d, members, hlist_index);

	hlist_index = 0;		# clear our stuff off the horizontal list.
	dis_stage = 3;
	} # end of Typesetter::DisNo()

void Typesetter::DisEnd(void)
	{
	if(dis_stage != 3)
		throw("DisEnd(): call order is DisStart(), DisPost(), DisNo(), DisEnd()");

	# Put the no-break text into an hbox
	Object *members = new Object[hlist_index];
	int x;
	SP w,h,d;
	w = h = d = PT(0);
	for(x=0; x < hlist_index; x++)
		{
		members[x] = hlist[x];
		w += hlist[x].GetWidth();
		if(hlist[x].GetHeight() > h) h = hlist[x].GetHeight();
		if(hlist[x].GetDepth() > d) d = hlist[x].GetDepth();
		}
	dis_no = new SuperBox(w,h,d,members,hlist_index);

	hlist_index = 0;	# these two commands
	pop_mode();			# are not redundant!
	dis_stage = 0;

	contribute_horizontal( (OType)disbreak, (void*) new Discretionary(dis_pre,dis_post,dis_no) );
	} # end of Typesetter::DisEnd()

#
# Insert a discretionary hyphen.
#
void Typesetter::DisHyphen(void)
	{
	DisStart();
	AddCharacter('-');
	DisPost();
	DisNo();
	DisEnd();
	} # end of Typesetter::DisHyphen()

#
# Emmit anything remaining
#
void Typesetter::End(void)
	{
	while(stackptr_mode)		# Make sure all remaining
		pop_mode();				# modes are closed.

	PostScript::End();			# Emmit buffered PostScript.
	} # end of Typesetter::End()

# end of file

