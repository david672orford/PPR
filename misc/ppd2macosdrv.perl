#! @PERL_PATH@ -w
#
# mouse:~ppr/src/misc/ppd2macosdrv.perl
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
# Last modified 18 April 2005.
#

$CONFDIR="@CONFDIR@";
$SHAREDIR="@SHAREDIR@";
$VAR_SPOOL_PPR="@VAR_SPOOL_PPR@";
$PPDDIR = "@PPDDIR@";
$DRVDIR = "$VAR_SPOOL_PPR/drivers/macos";
$PRINTERSDIR = "$CONFDIR/printers";
$TEMPFILE = "$DRVDIR/Temporary File";

#
# Parse the arguments:
#
$opt_verbose = 0;
foreach $arg (@ARGV)
	{
	if($arg eq '--verbose')
		{
		$opt_verbose = 1;
		open(STDERR, ">&1") || die;
		$| = 1;
		}
	else
		{ die "Unrecognized option: $arg\n" }
	}

# First include all of the PPD files distributed with PPR.
opendir(DIR, $PPDDIR) || die "Can't open directory \"$PPDDIR\", $!\n";
while(defined($file = readdir(DIR)))
	{
	next if($file =~ /\./);		# skip hidden files
	next if($file =~ /~$/);		# skip backup files
	next if($file =~ /\.bak$/i);

	$ppd_files{"$PPDDIR/$file"} = '';
	}
closedir(DIR) || die;

# Now, include the files used by the printers.	Generally these
# files will be some of the ones found in the step above, but
# some printers may be using PPD files outside PPR's ppd/
# directory.
opendir(DIR, $PRINTERSDIR) || die "Can't open directory \"$PRINTERSDIR\", $!\n";
while(defined($file = readdir(DIR)))
	{
	next if($file =~ /\./);		# skip hidden files
	next if($file =~ /~$/);		# skip backup files
	next if($file =~ /\.bak$/i);

	open(FILE, "<$PRINTERSDIR/$file") || die "Can't open \"$PRINTERSDIR/$file\", $!\n";
	while(<FILE>)
		{
		if(/^PPDFile:\s+(.+?)\s*$/)
			{
			$_ = $1;
			if(/^[^\/]/)
				{ $_ = "$PPDDIR/$_" }
			$ppd_files{$_} = '';
			last;
			}
		}
	close(FILE);
	}
closedir(DIR) || die;

# Create the output directory if it doesn't exist already.
mkdir($DRVDIR, 0755);

# For each PPD file, convert it to MS-DOS line termination
# and store it in the Win95 driver distribution share under
# its MS-DOS file name.
foreach $file (keys %ppd_files)
	{
	print "Processing \"$file\":\n" if($opt_verbose);

	open(OUT, ">$TEMPFILE") || die "Can't create \"$TEMPFILE\", $!\n";
	undef $macos_name;

	$inlevel = 'IN0';
	open($inlevel, "<$file") || die "Can't open \"$file\", $!\n";

	while(1)
		{
		$_ = <$inlevel>;
		if(!defined($_))
			{
			close($inlevel) || die;
			last if($inlevel eq 'IN0');
			print OUT "*% end of include\r\n";
			print "        End of include file.\n" if($opt_verbose);
			$inlevel =~ /^IN([0-9]+)$/;
			$inlevel = $1 - 1;
			$inlevel = "IN$inlevel";
			next;
			}

		chop;

		# If include file, choose a new file handle and
		# open it.
		if(/^\*Include:\s*"([^"]+)"/)
			{
			$include_file_name = $1;

			# If it isn't an absolute path,
			if(!/^\//)
				{
				# Use the directory of the including file
				# as the base path.
				($basepath) = $file =~ /^(.+)\//;

				$include_file_name = "$basepath/$include_file_name"
				}

			print "    Including \"$include_file_name\"...\n" if($opt_verbose);
			$inlevel++;
			open($inlevel, "<$include_file_name") ||
				die "Can't open include file \"$include_file_name\", $!\n";
			print OUT "*% $_\r";
			next;
			}

		# Copy the line to the output
		print OUT "$_\r";

		if( ! defined($macos_name) && $_ =~ /^\*Product:\s+"\(([^)]+)\)"/ )
			{ $macos_name = $1 }
		}

	close(OUT) || die;

	if( ! defined($macos_name) )
		{
		print STDERR "No \"*Product:\" line in \"$file\", skipping it.\n\n";
		unlink($TEMPFILE) || die;
		next;
		}

	print "    \"$macos_name\"\n" if($opt_verbose);

	# Encode slashes for Netatalk and CAP.
	$macos_name =~ s/\//:2F/g;

	# Renamed to temp file to be the Macintosh format PPD file.
	rename($TEMPFILE, "$DRVDIR/$macos_name") || die;

	print "\n" if($opt_verbose);
	} # end of PPD file iteration

print "Done.\n" if($opt_verbose);

exit 0;

