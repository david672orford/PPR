#
# mouse:~ppr/src/www/qquery_xlate.pl
# Copyright 1995--2001, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appears in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is"
# without express or implied warranty.
#
# Last modified 7 May 2001.
#

#
# This contains descriptions of the fields of the "ppop qquery"
# command.  These descriptions can be used as column headings
# in a GUI.
#
@qquery_fields = (
'jobname' => ['Job ID'],
'fulljobname' => ['Full Job ID'],
'-' => undef,
'status' => ['Status'],
'explain' => ['Explain'],
'status/explain' => ['Job Status'],
'-' => undef,
'subtime' => ['Time'],
'longsubtime' => ['Full Time'],
'-' => undef,
'for' => ['For Whom'],
'username' => ['Username'],
'userid' => ['UID'],
'proxy-for' => ['Proxy For'],
'-' => undef,
'lpqfilename' => ['File Name'],
'title' => ['Title'],
'creator' => ['Creator'],
'routing' => ['Routing Instructions'],
'-' => undef,
'pages' => ['Pages', "Number of PostScript Page Descriptions"],
'pagesxcopies', ['Pages', "Pages x Copies"],
'copies' => ['Copies', "Number of Copies to be Printed"],
'copiescollate' => ['Collate?', "Will the Copies be Collated?"],
'pagefactor' => ['Page Factor', "PostScript Page Descriptions Per Sheet"],
'nupn' => ['N-Up', "Number of Logical Pages Per Physical Page"],
'nupborders' => ['Borders?'],
'sigsheets' => ['Sig. Sheets'],
'sigpart' => ['Sig. Part'],
'totalpages', ['Total Pages'],
'totalsides', ['Total Sides'],
'totalsheets', ['Total Sheets'],
'-' => undef,
'priority' => ['Priority'],
'opriority' => ['Orig. Priority'],
'-' => ['Flag Pages'],
'banner' => ['Banner?'],
'trailer' => ['Trailer?'],
'-' => undef,
'inputbytes' => ['Size (input bytes)'],
'postscriptbytes' => ['Size (PS bytes)'],
'-' => undef,
'prolog' => ['Prolog Present?'],
'docsetup' => ['Docsetup Present?'],
'script' => ['Script Present?'],
'-' => undef,
'proofmode' => ['Proofmode'],
'orientation' => ['Orientation'],
'draft-notice' => ['Draft Notice'],
'pageorder' => ['Page Order']
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

