#! /usr/bin/perl
#! /usr/bin/perl -w
#
# mouse:~ppr/src/misc/custom_hook_docutech.perl
# Copyright 1995--2000, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is" without
# express or implied warranty.
#
# Last modified 22 September 2000.
#

use lib "?";
use 5.005;
use PPR;

#============================================================================
# Main
#============================================================================
my ($code, $parameter, $jobid) = @ARGV;
(defined($code) && defined($parameter) && defined($jobid)) || die "Wrong number of parameters!\n";

my $qentry = &parse_queue_entry($jobid);

if($code == 4)		# CUSTOM_HOOK_COMMENTS
    {
    &insert_header($qentry);
    }
elsif($code == 32)	# CUSTOM_HOOK_PAGEZERO
    {
    &insert_page($qentry);
    }

exit 0;

#============================================================================
# Parse the queue file and get the information we need.
#============================================================================
sub parse_queue_entry
{
my $jobid = shift;

my @fields = qw(
        title
	pages
	copies
	for
        routing
        addon:department
        addon:phone
	addon:accountnumber
        addon:datewanted
        addon:specialprocessing
	);

defined($PPR::PPOP_PATH) || die;
open(PPOP, "$PPR::PPOP_PATH -M qquery \"$jobid\" @fields |") || die "ppop failed, $!";
my $result_line = <PPOP>;
chomp $result_line;
close(PPOP) || die "ppop failed: $result_line";

my $qentry = {};
my $x = 0;
foreach my $value (split(/\t/, $result_line, 100))
    {
    $value =~ s/\(/\\(/g;
    $value =~ s/\)/\\)/g;
    $value =~ s/\\/\\\\/g;
    $qentry->{$fields[$x]} = $value;
    $x++;
    }

return $qentry;
}

#============================================================================
# Create the Docutech header lines
#============================================================================
sub insert_header
{
my $qentry = shift;

my $start_message = $qentry->{'addon:specialprocessing'};
$start_message =~ s/\\\\n/ /g;
$start_message = substr($start_message, 0, 128);

print <<"EndOfXeroxHeader";
%XRXbegin: 001.0300
%XRXPDLformat: PS-Adobe
%XRXaccount: $qentry->{'addon:accountnumber'}
%XRXtitle: $qentry->{title}
%XRXsenderName: $qentry->{for}
%XRXrecipientName: $qentry->{routing}
%XRXjobStartMessage: $start_message
%XRXmessage: $qentry->{'addon:phone'} $qentry->{'addon:department'}
%XRXdisposition: PRINT
%XRXend
EndOfXeroxHeader

print "%----------------- for debugging ------------------------\n";
foreach my $item (keys %$qentry)
    {
    print "%DEBUG: $item=\"$qentry->{$item}\"\n";
    }
print "%----------------- for debugging ------------------------\n";
}

#============================================================================
# Create the job ticket page
#============================================================================

# Subroutine to return the date
sub date
    {
    my $time_too = shift;
    my($min, $hour, $mday, $mon, $year) = (localtime(time()))[1,2,3,4,5];
    $mon++;
    $year += 1900;
    my $ampm = 'am';
    if($hour == 0)
    	{
    	$hour = 12;
    	}
    elsif($hour > 12)
    	{
	$hour -= 12;
    	$ampm = 'pm';
    	}
    if($time_too)
    	{ return sprintf("%d-%d-%d %d:%.2d %s", $mon, $mday, $year, $hour, $min, $ampm) }
    else
	{ return "$mon-$mday-$year" }
    }

sub insert_page
{
my $qentry = shift;

print <<"End_10";
%%Page: jobticket 0

% save the device context
save

%%BeginDocument: JobTicket
%!PS-Adobe 3.0
%%Title: Job Ticket
%%Creator: custom_hook_xerox.perl
%%EndComments

%%BeginProlog
/nl {/ypos ypos ls sub def lm ypos moveto} def
/hr { hrthick setlinewidth
	newpath
	lm hrhang sub ypos ls .3 mul add moveto
	pagewidth lm sub rm sub hrhang 2 mul add 0 rlineto
	closepath stroke
	nl} def
/beginpage { save
	/ypos pagelength tm sub ls sub def
	lm ypos moveto
	sf_norm } def
/endpage { restore showpage } def
/sf_norm { f_norm setfont } def
/sf_bold { f_bold setfont } def
/sf_title { f_title setfont } def
/field { sf_bold       		% stack: name value width
	3 -1 roll		% stack: value width name
	show			% stack: value width
	(: ) show		% name, value separator is colon and space
	currentpoint		% stack: value width x y

	ulthick setlinewidth
	2 copy newpath				% draw
	ls uldrop mul sub moveto		% the
	2 index 0 rlineto closepath stroke	% underline

	2 copy moveto
	4 -1 roll			% stack: width x y value
	sf_norm
	show				% stack: width x y

	exch 3 -1 roll add exch moveto	% move to end of underline
	} def
/hs { currentpoint
	3 1 roll
	add
	exch
	moveto } def
%%EndProlog

%%BeginSetup

% Setup up output device state.  This is commented out because
% the Docutech doesn't restore the device state properly when
% the restore is executed.  Thus we don't want to change the
% device state as that would mess up the document.
%
% %%IncludeFeature: *InputSlot AutoSelect
% %%IncludeFeature: *PageSize Letter
% %%IncludeFeature: *MediaType Plain
% %%IncludeFeature: *MediaColor white
% %%IncludeFeature: *Collate False
% %%IncludeFeature: *Duplex None
% %%IncludeFeature: *XRXFinishing None
initmatrix

% Set initial values of appearance parameters
/tsize 12 def
/inch {72 mul} def
/pagelength 11.0 inch def
/pagewidth 8.5 inch def
/tm 0.75 inch def
/bm 0.75 inch def
/lm 0.75 inch def
/rm 0.75 inch def
/ls tsize 1.20 mul def
/hrhang 0.25 inch def
/hrthick 1.5 def
/uldrop 0.15 def
/ulthick 1 def

% Load fonts
%%IncludeResource: font Times-Roman
%%IncludeResource: font Times-Bold
%%IncludeResource: font Helvetica-Bold
/f_norm /Times-Roman findfont tsize scalefont def
/f_bold /Times-Bold findfont tsize scalefont def
/f_title /Helvetica-Bold findfont tsize 2.0 mul scalefont def

%%EndSetup

%%Page: 1 1
beginpage

% That thing on the top right:
lm ypos
/lm 5.0 inch def
lm ypos moveto
(Refer to job #) show nl
(when inquiring) show nl
(about work) show nl
(in progress.) show
	0.25 inch hs
	(Job #) () 1.0 inch field
	nl
/ypos exch def
/lm exch def
lm ypos moveto

% Increase line spacing
/ls ls 1.5 mul def

sf_title (Printing Requisition) show nl
(Central Services) show nl
nl
nl

(Account) ($qentry->{'addon:accountnumber'}) 1.5 inch field
	0.25 inch hs
	(Date) (${\&date()}) 1.5 inch field
	0.25 inch hs
	(Date wanted) ($qentry->{'addon:datewanted'}) 1.5 inch field
	nl

(Name) ($qentry->{for}) 2.0 inch field
	0.25 inch hs
	(Telephone) ($qentry->{'addon:phone'}) 1.5 inch field
	nl

(Department) ($qentry->{'addon:department'}) 4.0 inch field
	nl

(Print job title) ($qentry->{title}) 5.0 inch field
	nl

hr
sf_title (INSTRUCTIONS) show nl

	(Pages) ($qentry->{pages}) 0.5 inch field
	0.25 inch hs
	(Copies) ($qentry->{copies}) 0.5 inch field
	nl

(Delivery instructions) ($qentry->{routing}) 4.0 inch field
	nl

hr
sf_title (Special Processing Instructions) show nl

End_10

my $x = 1;
my @lines = split(/\\\\n/, $qentry->{'addon:specialprocessing'});
while(scalar(@lines) < 6)
    {
    push(@lines, "");
    }
foreach $line (@lines)
    {
    print "($x) ($line) 6 inch field nl\n";
    $x++;
    }
print <<"End_80";
% Restore line spacing
/ls ls 1.5 div def

hr
(THIS SECTION FOR CENTRAL SERVICES USE ONLY) show nl
nl
(Operator's initials) () 0.75 inch field nl
(Actual time) () 0.75 inch field nl
(Date completed) () 0.75 inch field nl
(Total impressions) () 0.75 inch field nl
End_80

print <<"End_90";
endpage

%%Trailer
%%EOF
%%EndDocument

% restore device context
restore

End_90
}

# end of file

