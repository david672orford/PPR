#! /usr/bin/perl -w
#
# mouse:~ppr/src/www/q_icons/make_composit_figs.perl
# Copyright 1995--2000, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appears in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is"
# without express or implied warranty.
#
# Last modified 29 December 2000.
#

#
# Write a Fig file header.
#
sub write_header
{
my $handle = shift;
my $mag = shift;

print $handle <<"EndHeader";
#FIG 3.2
Landscape
Center
Inches
Letter
$mag
Single
-2
1200 2
0 32 #8e8e8e
EndHeader
}

#
# Write a Fig file body that combines several Fig files.
#
sub write_file_body
{
my $handle = shift;
my $files = shift;
my @list;

if(ref($files) eq 'ARRAY')
    {
    @list = @$files;
    }
elsif($files eq '')
    {
    return;
    }
else
    {
    @list = ($files);
    }

my $file;
foreach $file (@list)
    {
    print " $file ";
    open(F, "<$file") || die "Can't open \"$file\", $!";
    while(<F>)
        {
        if(/^[1-5] /)
            { $skip = 0; print $handle $_; }
        elsif(/^\t/)
            { if(!$skip) { print $handle $_ } }
        else
            { $skip = 1 }

        }
    close(F) || die;
    }
}

#
# This data describes how we should combine Fig files to create composit
# Fig files.
#
$filename_map = [
	[			# type of object
	'printer1.fig',
	'group1.fig'
	],
	[			# accepting or rejecting new jobs
	'',
	'rejecting.fig'
	],
	[			# charging money for printing?
	'',
	'charge.fig'
	],
	[			# jobs in queue?
	'',
	'queued.fig',
	],
	[			# printer status
	'',            				# 0 idle
	'printing.fig',				# 1 printing
	['stopt.fig', 'printing.fig'],		# 2 stopping
	'canceling.fig',			# 3 canceling job
	'stopt.fig',				# 4 stopt
	'fault2.fig',				# 5 fault
	'fault3.fig',				# 6 fault, no auto retry
	'engaged.fig',				# 7 otherwise engaged or off-line
	'offline.fig',				# 8 definitely off-line
	'need.fig',				# 9 in need (out of paper?)
	['printing.fig', 'offline.fig'],	# a printing but off-line
	['printing.fig', 'need.fig']		# b printing but in need
	]
];

#
# Main
#
$| = 1;
my($type, $accepting, $charge, $queued, $status);
for($type=0; $type <= 1; $type++)
  {
  for($accepting=0; $accepting <= 1; $accepting++)
    {
    for($charge=0; $charge <= 1; $charge++)
      {
      for($queued=0; $queued <= 1; $queued++)
        {
        for($status=0; $status <= 11; $status++)
          {
	  # Groups don't have complicated status
	  next if($type > 0 && $status > 0);

	  my $hstatus = (qw(0 1 2 3 4 5 6 7 8 9 a b c d e f))[$status];
	  my $basic_name = "$type$accepting$charge$queued$hstatus";

	  # Create a new Fig file.
	  print "$basic_name (";
	  open(OUT, ">$basic_name.fig") || die;

	  # Write a header while setting the scaling factor.
	  write_header(OUT, 30.0);

	  # Copy the bodies of several Fig files.
	  write_file_body(OUT, "boundingbox.fig");
	  write_file_body(OUT, $filename_map->[0]->[$type]);
	  write_file_body(OUT, $filename_map->[1]->[$accepting]);
	  write_file_body(OUT, $filename_map->[2]->[$charge]);
	  write_file_body(OUT, $filename_map->[3]->[$queued]);
	  write_file_body(OUT, $filename_map->[4]->[$status]);

	  # Close the new Fig file.
	  close(OUT) || die;
	  print ")\n";

	  unlink("$basic_name.png");

	  # Obvious but produces images of various sizes:
	  #system("fig2dev -L png $basic_name.fig >$basic_name.png") && die;

	  # Very basic ImageMagic:
	  #system("convert $basic_name.fig $basic_name.png") && die;

	  # Here we use ImageMagick and try to get a transparent background:
	  #system("convert -quality 100 -transparent white $basic_name.fig $basic_name.png") && die;

	  # ImageMagick with reduced colors:
	  #system("convert -colors 64 -quality 100 $basic_name.fig $basic_name.png") && die;

	  # Does this achieve the web palete?
	  #system("convert -colors 216 -quality 100 $basic_name.fig $basic_name.png") && die;

	  # Was almost right, but broken in Linux Mandrake 7.2:
	  #system("montage -quality 100 -gravity SouthWest -background white -transparent white -geometry 95x93 $basic_name.fig $basic_name.png") && die;

	  # Combine fig2dev and ImageMagic:
	  #system("fig2dev -L png $basic_name.fig >temp.png && montage -quality 100 -gravity SouthWest -background white -transparent white -geometry 95x93 temp.png $basic_name.png && rm temp.png") && die;

	  # ImageMagic with PBM utilities:
	  system("convert $basic_name.fig $basic_name.ppm") && die;
	  #system("pnmflip -cw $basic_name.ppm | pnmcrop -white | pnmtopng -compression 9 >$basic_name.png") && die;
	  system("pnmflip -cw $basic_name.ppm | pnmcut 352 262 88 88 | pnmtopng -compression 9 >$basic_name.png") && die;
	  unlink("$basic_name.ppm") || die $!;

	  unlink("$basic_name.fig") || die $!;
          }
        }
      }
    }
  }

exit 0;

