#
# mouse:~ppr/src/www/ppd_select.pl
# Copyright 1995--2004, Trinity College Computing Center.
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
# Last modified 26 May 2004.
#

defined($PPD_INDEX) || die;
defined($PPDDIR) || die;
defined($PPAD_PATH) || die;

require 'cgi_run.pl';

#
# This returns a list of two item lists.  Items are Manufacturer and ModelName.
#
sub ppd_list
	{
	my $cached = shift;
	my @list = ();

	# If we have been passed an old list, just decode it and return it.
	if(defined $cached)
		{
		@list = ppd_list_decode($cached);
		}

	# Next try the PPD index.
	elsif(open(P, $PPD_INDEX))
		{
		while(my $line = <P>)
			{
			next if($line =~ /^#/);
			chomp $line;
			my($description, $manufacturer) = (split(/:/, $line))[0, 2];
			next if($manufacturer eq "PPR");		# these aren't for printers
			push(@list, [$manufacturer, $description]);
			}
		close(P) || die $!;
		}

	# Finally, just use a directory listing.
	else
		{
		opendir(P, $PPDDIR) || die "opendir() failed on \"$PPDDIR\", $!";
		while(my $file = readdir(P))
			{
			next if($file =~ /^\./);
			next if($file =~ /^PPR Generic/);		# these aren't for printers
			push(@list, ["", $file]);
			}
		closedir(P) || die $!;
		}

	@list = sort { my $x = ($a->[0] cmp $b->[0]); if($x) { $x } else {$a->[1] cmp $b->[1]} } @list;
	return @list;
	} # ppd_list()

#
# This function prints a table with a brief summary of the contents
# of a PPD file.
#
sub ppd_summary
	{
	require "readppd.pl";
	my($ppdname) = @_;

	eval {
		my $line;
		my $modelname = "?";
		my $fileversion = "?";
		my $languagelevel = 1;
		my $psversion = "?";
		my $fonts = 0;
		my $ttrasterizer = "None";
		my $rip = "Internal";
	
		my $filename = ppd_open($ppdname);
	
		while(defined($line = ppd_readline()))
			{
			if($line =~ /^\*ModelName:\s*"([^"]+)"/)
				{
				$modelname = $1;
				next;
				}
			if($line =~ /^\*FileVersion:\s*"([^"]+)"/)
				{
				$fileversion = $1;
				next;
				}
			if($line =~ /^\*LanguageLevel:\s*"([^"]+)"/)
				{
				$languagelevel = $1;
				next;
				}
			if($line =~ /^\*PSVersion:\s*"([^"]+)"/)
				{
				$psversion = $1;
				next;
				}
			if($line =~ /^\*Font\s+/)
				{
				$fonts++;
				next;
				}
			if($line =~ /^\*TTRasterizer:\s*(\S+)/)
				{
				$ttrasterizer = $1;
				next;
				}
			if($line =~ /^\*cupsFilter:\s*"([^"]+)"/)
				{
				my $options = $1;
				my @options = split(/ /, $options);
				$rip = "Ghostscript+CUPS";
				$rip .= "+GIMP" if($options[2] eq "rastertoprinter");
				$rip .= "+GIMP" if($options[2] =~ /^rastertogimpprint/);
				next;
				}
			if($line =~ /^\*pprRIP:\s*(.+)$/)
				{
				my $options = $1;
				my @options = split(/ /, $options);
				$rip = "Ghostscript";
				if(grep(/^-sDEVICE=cups$/, @options))
					{ $rip .= "+CUPS" }
				elsif(my($temp) = grep(s/^-sDEVICE=(.+)$/$1/, @options))
					{ $rip .= " ($temp)" }
				$rip .= "+GIMP" if(grep(/^cupsfilter=rastertoprinter$/, @options));
				next;
				}
			}
	
		print "<table class=\"lines\" cellspacing=0>\n";
		$filename = html($filename);
		$filename =~ s/\//\/<wbr>/g;
		print "<tr><th>", H_("PPD File"), "</th><td>", $filename, "</td></tr>\n";
		print "<tr><th>", H_("ModelName"), "</th><td>", html($modelname), "</td></tr>\n";
		print "<tr><th>", H_("PPD File Version"), "</th><td>", html($fileversion), "</td></tr>\n";
		print "<tr><th>", H_("LanguageLevel"), "</th><td>", html($languagelevel), "</td></tr>\n";
		print "<tr><th>", H_("PostScript Version"), "</th><td>", html($psversion), "</td></tr>\n";
		print "<tr><th>", H_("Number of Fonts"), "</th><td>", html($fonts), "</td></tr>\n";
		print "<tr><th>", H_("TrueType Rasterizer"), "</th><td>", html($ttrasterizer), "</td></tr>\n";
		$rip = html($rip);
		$rip =~ s/\+/<wbr>+/g;
		print "<tr><th>", H_("RIP"), "</th><td>", $rip, "</td></tr>\n";
		print "</table>\n";
		};
	if($@)
		{
		my $error = $@;
		print "<p>", html($error), "</p>\n";
		}
	} # ppd_summary()

#
# This function probes the printer and returns a list of suitable 
# PPD files.  The list might be empty.  If the printer can't be
# probed for any reason, undef will be returned.
#
sub ppd_probe
	{
	my($interface, $address, $options) = @_;

	print STDERR "ppd_probe(\"$interface\", \"$address\", \"$options\")\n";

	opencmd(RESULTS, "2>&STDERR", $PPAD_PATH, "-M", "ppdlib", "query", $interface, $address, $options);

	my @list = ();
	while(<RESULTS>)
		{
		chomp;
		push(@list, $_);
		}
		
	if(!close(RESULTS))
		{
		print STDERR "ppad failed, \$?=$?\n";
		return undef;
		}

	return join("\n", @list);
	} # ppd_probe()

#
# This function takes the value returned by ppd_probe() and breaks
# it up into a list.
#
sub ppd_list_decode
	{
	my $ppd_detect_list = shift;
	my @list = ();
	foreach my $i (split(/\n/, $ppd_detect_list))
		{
		my($mfg, $mdl, $fuzzy) = split(/:/,$i);
		push(@list, [$mfg, $mdl, $fuzzy]);
		}
	return @list;
	} # ppd_list_decode()

1;
