#! /usr/bin/perl -w
#
# mouse:~ppr/src/www/q_icons/make_composit_figs.perl
# Copyright 1995--2002, Trinity College Computing Center.
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
# Last modified 21 November 2002.
#

my $SCALING_FACTOR = 30.0;

#
# Write a Fig file header.
#
sub fig_write_header
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
sub fig_write_body
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

    foreach my $file (@list)
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
# Convert a FIG file to a PNG file.
#
sub fig_to_png
    {
    my $fig_name = shift;
    my $png_name = shift;
    
    unlink("$png_name");

    # Obvious but produces images of various sizes:
    #system("fig2dev -L png $fig_name >$png_name") && die;

    # Very basic ImageMagic:
    #system("convert $fig_name $png_name") && die;

    # Here we use ImageMagick and try to get a transparent background:
    #system("convert -quality 100 -transparent white $fig_name $png_name") && die;

    # ImageMagick with reduced colors:
    #system("convert -colors 64 -quality 100 $fig_name $png_name") && die;

    # Does this achieve the web palete?
    #system("convert -colors 216 -quality 100 $fig_name $png_name") && die;

    # Was almost right, but broken in Linux Mandrake 7.2:
    #system("montage -quality 100 -gravity SouthWest -background white -transparent white -geometry 95x93 $fig_name $png_name") && die;

    # Combine fig2dev and ImageMagic:
    #system("fig2dev -L png $fig_name >temp.png && montage -quality 100 -gravity SouthWest -background white -transparent white -geometry 95x93 temp.png $png_name && rm temp.png") && die;

    # ImageMagic with PBM utilities:
    system("convert $fig_name temp.ppm") && die;
    system("pnmflip -cw temp.ppm | pnmcut 352 262 88 88 | pnmtopng -transparent white -compression 9 >$png_name") && die;
    unlink("temp.ppm") || die $!;
    }

#
# This data describes how we should combine Fig files to create composit
# Fig files.
#
$filename_map = [
	[			# type of object
	'printer1.fig',
	'group1.fig',
	'alias1.fig'
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

#=============================================================================
# Main
#=============================================================================

$| = 1;

# Do the printer and group icons.
for(my $type=0; $type <= 1; $type++)
  {
  for(my $accepting=0; $accepting <= 1; $accepting++)
    {
    for(my $charge=0; $charge <= 1; $charge++)
      {
      for(my $queued=0; $queued <= 1; $queued++)
        {
        for(my $status=0; $status <= 11; $status++)
          {
	  # Groups don't have complicated status, so skip those images 
	  # which don't apply.
	  next if($type > 0 && $status > 0);

	  my $hstatus = (qw(0 1 2 3 4 5 6 7 8 9 a b c d e f))[$status];
	  my $basic_name = "$type$accepting$charge$queued$hstatus";

	  # Create a new Fig file.
	  print "$basic_name (";
	  open(OUT, ">$basic_name.fig") || die;

	  # Write a header while setting the scaling factor.
	  fig_write_header(OUT, $SCALING_FACTOR);

	  # Copy the bodies of several Fig files.
	  fig_write_body(OUT, "boundingbox.fig");
	  fig_write_body(OUT, $filename_map->[0]->[$type]);
	  fig_write_body(OUT, $filename_map->[1]->[$accepting]);
	  fig_write_body(OUT, $filename_map->[2]->[$charge]);
	  fig_write_body(OUT, $filename_map->[3]->[$queued]);
	  fig_write_body(OUT, $filename_map->[4]->[$status]);

	  # Close the new Fig file.
	  close(OUT) || die;
	  print ")\n";

	  fig_to_png("$basic_name.fig", "$basic_name.png");

	  unlink("$basic_name.fig") || die $!;
          }
        }
      }
    }
  }

# Do the alias icon.
print "20000 (";
open(OUT, ">20000.fig") || die;
fig_write_header(OUT, $SCALING_FACTOR);
fig_write_body(OUT, "boundingbox.fig");
fig_write_body(OUT, "alias1.fig");
close(OUT) || die $!;
fig_to_png("20000.fig", "20000.png");
unlink("20000.fig") || die $!;
print ")\n";

exit 0;

