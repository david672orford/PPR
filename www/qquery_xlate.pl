#
# mouse:~ppr/src/www/qquery_xlate.pl
# Copyright 1995--2005, Trinity College Computing Center.
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
# Last modified 14 October 2005.
#

require 'cgi_intl.pl';

#
# This contains descriptions of the fields of the "ppop qquery"
# command.	These descriptions can be used as column headings
# in a GUI.
#
# Column 1: field name according to "ppop qquery"
# Column 2: name to display in queue listing
# Column 3: longer name for tooltip
# Column 4: minimum width (optional)
#
@qquery_fields = (
'jobname' =>			[N_('Job ID'),			N_("Friendly Form of the Job ID"), 13],
'fulljobname' =>		[N_('Full Job ID'),		N_("Elaborate, Fully Qualified Form of the Job ID")],
'-' => undef,
'status' =>				[N_('Status'),			N_("Status of the Job")],
'explain' =>			[N_('Explain'),			N_("Elaboration on Status of the Job")],
'status/explain' =>		[N_('Job Status'),		N_("Status of the Job with Elaboration")],
'-' => undef,
'subtime' =>			[N_('Time'),			N_("Job Submission Time, Compact")],
'longsubtime' =>		[N_('Full Time'),		N_("Job Submission Time, Full")],
'-' => undef,
'for' =>				[N_('For Whom'),		N_("Whose Job is This?")],
'username' =>			[N_('Username'),		N_("Unix Username of Job Submitter")],
'userid' =>				[N_('UID'),				N_("Numberic Unix Userid of Job Submitter")],
'-' => undef,
'lpqfilename' =>		[N_('File Name'),		N_("What Was the Name of the Input File?")],
'title' =>				[N_('Title'),			N_("Descriptive Job Title"), 20],
'creator' =>			[N_('Creator'),			N_("Person or Application that Created the Job"), 20],
'routing' =>			[N_('Routing Instructions'), N_("Instructions to a Human Operator for Delivery of the Output")],
'-' => undef,
'pages' =>				[N_('Pages'),			N_("Number of PostScript Page Descriptions")],
'pagesxcopies' =>		[N_('Pages'),			N_("Pages x Copies")],
'copies' =>				[N_('Copies'),			N_("Number of Copies to be Printed")],
'copiescollate' =>		[N_('Collate?'),		N_("Will the Copies be Collated?")],
'pagefactor' =>			[N_('Page Factor'),		N_("PostScript Page Descriptions Per Sheet")],
'nupn' =>				[N_('N-Up'),			N_("Number of Logical Pages Per Physical Page")],
'nupborders' =>			[N_('Borders?'),		N_("Print Borders Around Logical Pages in N-Up Mode?")],
'sigsheets' =>			[N_('Sig. Sheets'),		N_("How Many Sheets Per Signiture?  (For Booklet Printing.)")],
'sigpart' =>			[N_('Sig. Part'),		N_("The Fronts, the Backs, or Both")],
'totalpages' =>			[N_('Total Pages'),		N_("Total Page Descriptions for All Copies")],
'totalsides' =>			[N_('Total Sides'),		N_("Total Medium Sides to be Marked (after N-Up)")],
'totalsheets' =>		[N_('Total Sheets'),	N_("Total Medium Sheets to be Used")],
'-' => undef,
'priority' =>			[N_('Priority'),		N_("Current Queue Priority of this Job")],
'opriority' =>			[N_('Orig. Priority'),	N_("Original Queue Priority of this Job")],
'-' => undef,
'banner' =>				[N_('Banner?'),			N_("Was a Banner Page Requested?")],
'trailer' =>			[N_('Trailer?'),		N_("Was a Trailer Page Requested?")],
'-' => undef,
'inputbytes' =>			[N_('Size (input bytes)'), N_("Size of Input File Before Conversion to PostScript")],
'postscriptbytes' =>	[N_('Size (PS bytes)'), N_("Size of Input File After Conversion to PostScript")],
'-' => undef,
'prolog' =>				[N_('Prolog Present?'), N_("Does this Document have a DSC Prolog?")],
'docsetup' =>			[N_('Docsetup Present?'), N_("Does this Document have a DSC Document Setup Section?")],
'script' =>				[N_('Script Present?'), N_("Does this Document have a DSC Script?")],
'-' => undef,
'proofmode' =>			[N_('Proofmode'),		N_("DSC Proofmode Setting")],
'orientation' =>		[N_('Orientation'),		N_("Portrait or Landscape")],
'draft-notice' =>		[N_('Draft Notice'),	N_("Message to Print Diagonally Across Each Page")],
'pageorder' =>			[N_('Page Order'),		N_("Are the Pages in Ascending or Descending Order in the Input File?")]
);

%qquery_xlate = ();
@qquery_available = ();
for(my $x = 0; $x < $#qquery_fields; $x += 2)
	{
	my $i = $qquery_fields[$x];
	push(@qquery_available, $i);
	next if($i eq '-');
	$qquery_xlate{$i} = $qquery_fields[$x + 1];
	}

1;
