#! @PERL_PATH@ -w
#
# mouse:~ppr/src/samba/ppd2windrv.perl
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
# Last modified 13 January 2005.
#

#
# This script is used to prepare a Samba shared directory for Win95 driver
# installations.  This script should run with Perl 4 or 5.
#

$CONFDIR="@CONFDIR@";
$HOMEDIR="@HOMEDIR@";
$VAR_SPOOL_PPR="@VAR_SPOOL_PPR@";

# This is the directory which contains the PPR printer configuration files.
$PRINTERSDIR = "$CONFDIR/printers";

# This is the directory which contains PPR's collection of PPD files.
$PPD_INDEX = "$VAR_SPOOL_PPR/ppdindex.db";

# These are the directories from which clients download drivers.
$DRVDIR = "$VAR_SPOOL_PPR/drivers";
$DRVDIR_SHARE = "\\PPRPRNT\$";
$W32X86 = "W32X86";								# Windows NT x86
$WIN40 = "WIN40";								# Windows 4.0 (95)
$WINPPD = "WINPPD";								# PPD files in MS-DOS text format
$DRVDIR_W32X86 = "$DRVDIR/$W32X86";
$DRVDIR_WIN40 = "$DRVDIR/$WIN40";
$DRVDIR_WINPPD = "$DRVDIR/$WINPPD";

# We use this when copying in new PPD files.
$TEMP_PPD_FILE = "$DRVDIR_WINPPD/NEW";

# List of files for the MS-Windows 4.0 (95) PostScript driver.	This driver
# can drive both level 1 and level 2 PostScript printers.
@FILES_WIN40_MS = qw(PSCRIPT.DRV PSCRIPT.HLP FONTS.MFM ICONLIB.DLL PSMON.DLL PSCRIPT.INI);

# List of files for the Adobe version 4.1 PostScript driver for Windows 95.
# This driver can drive both level 1 and level 2 PostScript printers.
@FILES_WIN40_ADOBE_4_1 = qw(ADBEPS41.DRV ADOBEPS4.HLP FONTS.MFM ICONLIB.DLL PSMON.DLL);

# List of files for the Adobe version 4.2 PostScript driver.  This driver
# can drive level 2 but not level 1 PostScript printers.
@FILES_WIN40_ADOBE_4_2 = qw(ADOBEPS4.DRV ADOBEPS4.HLP ADFONTS.MFM ICONLIB.DLL PSMON.DLL PSCRIPT.INI);

# List of files for the MS-Windows NT 4.0 PostScript driver.
@FILES_X32X86_2_MS = qw(2/PSCRIPT.DLL 2/PSCRPTUI.DLL 2/PSCRIPT.HLP);

# List of files for the Adobe version 5.x PostScript driver for Windows NT 4.0
@FILES_W32X86_2_ADOBE_5 = qw(2/ADOBEPS5.DLL 2/ADOBEPSU.DLL 2/ADOBEPSU.HLP 2/ADOBEPS5.NTF);

# List of files for the Windows NT 5.0 PostScript driver.
@FILES_W32X86_3_MS = qw(3/PSCRIPT5.DLL 3/PS5UI.DLL 3/PSCRIPT.HLP 3/PSCRIPT.NTF);

# List of files for the Adobe 5.x PostScript driver for Windows NT 5.0.
@FILES_W32X86_3_ADOBE_5 = qw(3/ADOBEPS5.DLL 3/ADOBEPSU.DLL 3/ADOBEPSU.HLP 3/ADOBEPS5.NTF);

#
# Parse the arguments:
#
$opt_verbose = 0;
foreach $arg (@ARGV)
	{
	if($arg eq '--verbose')
		{
		$opt_verbose = 1;
		}
	else
		{ die "Unrecognized option: $arg\n" }
	}

#
# Create the output directory if it doesn't already exist.
#
mkdir($DRVDIR_WIN40, 0755);		# Win95
mkdir($DRVDIR_W32X86, 0755);	# WinNT
mkdir("$DRVDIR_W32X86/2", 0755);
mkdir($DRVDIR_WINPPD, 0755);	# PPD Files

#
# Convert all of the filenames in the driver directories to upper case.
#
foreach my $dir ($DRVDIR_WIN40, $DRVDIR_W32X86, "$DRVDIR_W32X86/2")
	{
	print STDERR "Changing filenames in \"$dir\" to upper case...\n" if($opt_verbose);
	opendir(DIR, $dir) || die "Can't open directory \"$dir\", $!\n";
	while(defined($file = readdir(DIR)))
		{
		next if($file =~ /^\./);

		$upper = $file;
		$upper =~ tr/[a-z]/[A-Z]/;
		if($upper ne $file)
			{
			print STDERR "\t$file -> $upper\n" if($opt_verbose);
			if(!rename("$dir/$file", "$dir/$upper"))
				{
				die "Can't rename \"$dir/$file\" to \"$dir/$upper\", $!\n";
				}
			}
		}
	closedir(DIR);
	print STDERR "Done.\n\n" if($opt_verbose);
	}

#
# This function is used to figure out which MS-Windows drivers are
# installed.  It is passed a list of files.	 If all of them
# exist, then it returns true, otherwise it returns false.
# The first argument is a text string describing the driver
# whose files we are looking for.  The rest of the arguments
# are the list of files.
#
sub allfound
	{
	my $description = shift;
	my $directory = shift;
	my $result = 1;
	print STDERR "Looking for $description...\n" if($opt_verbose);
	foreach $file (@_)
		{
		print STDERR "\t\t$file" if($opt_verbose);
		if(-f "$directory/$file")
			{
			print STDERR ", found\n" if($opt_verbose);
			}
		else
			{
			print STDERR ", not found\n" if($opt_verbose);
			$result = 0;
			}
		}
	if($opt_verbose)
		{
		print STDERR "\tUsable driver ";
		print STDERR "not " if(!$result);
		print STDERR "present.\n\n";
		}
	return $result;
	}

$HAVE_WIN40_MS = allfound("Win95 Microsoft driver", $DRVDIR_WIN40, @FILES_WIN40_MS);
$HAVE_WIN40_ADOBE_4_1 = allfound("Win95 Adobe driver 4.1", $DRVDIR_WIN40, @FILES_WIN40_ADOBE_4_1);
$HAVE_WIN40_ADOBE_4_2 = allfound("Win95 Adobe driver 4.2x", $DRVDIR_WIN40, @FILES_WIN40_ADOBE_4_2);
$HAVE_X32X86_2_MS = allfound("WinNT 4.0 Microsoft driver", $DRVDIR_W32X86, @FILES_X32X86_2_MS);
$HAVE_W32X86_2_ADOBE_5 = allfound("WinNT 4.0 Adobe driver 5.x", $DRVDIR_W32X86, @FILES_W32X86_2_ADOBE_5);
$HAVE_W32X86_3_MS = allfound("WinNT 5.0 Microsoft driver", $DRVDIR_W32X86, @FILES_W32X86_3_MS);
$HAVE_W32X86_3_ADOBE_5 = allfound("WinNT 5.0 Adobe Drivers", $DRVDIR_W32X86, @FILES_W32X86_3_ADOBE_5);

#
# Convert the PPD files and print the driver descriptions.
#
if($opt_verbose)
	{
	print STDERR "Converting PPD files...\n";
	}

# List of PPD files to copy into the driver distribution share.
%ppd_files = ();

# First include all of the PPD files indexed by PPR.
open(INDEX, $PPD_INDEX) || die "Can't open index file \"$PPD_INDEX\", $!\n";
while(defined(my $line = <INDEX>))
	{
	next if($line =~ /^#/);		# skip comments (in header)
	my($filename) = (split(/:/, $line))[1];
	defined $filename || die "bad index";
	-f $filename || die "index is out of date";
	$ppd_files{$filename} = '';
	}
close(INDEX) || die;

# Now, include any files mentioned by absolute paths in the printer
# configuration files.
opendir(DIR, $PRINTERSDIR) || die "Can't open directory \"$PRINTERSDIR\", $!\n";
while(defined($file = readdir(DIR)))
	{
	next if($file =~ /\./);		# skip hidden files
	next if($file =~ /~$/);		# skip backup files
	next if($file =~ /\.bak$/i);

	open(FILE, "<$PRINTERSDIR/$file") || die "Can't open \"$PRINTERSDIR/$file\", $!\n";
	while(my $line = <FILE>)
		{
		if($line =~ /^PPDFile:\s+(.+?)\s*$/)
			{
			my $ppd_file = $1;
			if($ppd_file =~ m#^/# && -f $ppd_file)
				{
				$ppd_files{$ppd_file} = '';
				}
			last;
			}
		}
	close(FILE);
	}
closedir(DIR) || die;

# Open sambaprint to accept the data.
open(DRIVERS, "| $HOMEDIR/lib/sambaprint drivers import") || die $!;

# For each PPD file, convert it to MS-DOS line termination and store it in the
# WINPPD subdirectory of the driver distribution share under its MS-DOS file
# name.
foreach $file (keys %ppd_files)
	{
	print STDERR "Processing \"$file\":\n" if($opt_verbose);

	open(OUT, ">$TEMP_PPD_FILE") || die "Can't create \"$TEMP_PPD_FILE\", $!\n";
	undef $mswin_name;
	undef $nickname;
	undef $languagelevel;

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
			print STDERR "\t    End of include file.\n" if($opt_verbose);
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

			print STDERR "\tIncluding \"$include_file_name\"...\n" if($opt_verbose);
			$inlevel++;
			open($inlevel, "<$include_file_name") ||
				die "Can't open include file \"$include_file_name\", $!\n";
			print OUT "*% $_\r\n";
			next;
			}

		# Copy the line to the output
		print OUT "$_\r\n";

		# If this is the first PC file name line take it, but we
		# ignore it if it is not in the outermost file.	 This
		# rule isn't in the PPD spec, but it seems right.
		if(! defined($mswin_name) && /^\*PCFileName:\s+"([^"]+)"/ && $inlevel eq 'IN0')
			{
			$mswin_name = $1;
			$mswin_name =~ tr/a-z/A-Z/;
			}

		# Take first "*NickName:" or "*ShortNickName:" line.  The PPD
		# spec says that the "*ShortNickName:" must be first if it
		# is present.
		if(! defined($nickname) && /^\*(Short)?NickName:\s+"([^"]+)"/)
			{
			$nickname = $2;
			}

		# Take first "*LanguageLevel:" line.
		if(! defined($languagelevel) && /^\*LanguageLevel:\s+"([0-9]+)"/)
			{
			$languagelevel = $1;
			}

		}

	close(OUT) || die;

	if( ! defined($mswin_name) )
		{
		print STDERR "No \"*PCFileName:\" line in \"$file\", skipping it.\n\n";
		unlink($TEMP_PPD_FILE) || die;
		next;
		}

	if( ! defined($nickname) )
		{
		print STDERR "\tNo *ShortNickName: or \"*NickName:\" line in \"$file\", skipping it.\n\n";
		unlink($TEMP_PPD_FILE) || die;
		next;
		}

	if( ! defined($languagelevel) )
		{
		print STDERR "\tWarning: no \"*LanguageLevel:\" line in \"$file\", assuming 1.\n";
		$languagelevel = 1;
		}

	if($opt_verbose)
		{
		print STDERR "\tPC File Name: $mswin_name\n";
		print STDERR "\tDriver Name: $nickname\n";
		print STDERR "\tLanguage Level: $languagelevel\n";
		}

	rename($TEMP_PPD_FILE, "$DRVDIR_WINPPD/$mswin_name") || die;

	#=================================================================
	# Windows 95 driver
	#=================================================================
	{
	my @filelist = ();

	# If we can use the latest Adobe driver, do so.
	if($languagelevel > 1 && $HAVE_WIN40_ADOBE_4_2)
		{
		print STDERR "\tWin95 driver chosen: Adobe 4.2.x\n" if($opt_verbose);
		@filelist = @FILES_WIN40_ADOBE_4_2;
		}
	# If we have a renamed copy of the Adobe 4.1 driver,
	# use it.
	elsif($HAVE_WIN40_ADOBE_4_1)
		{
		print STDERR "\tWin95 driver chosen: Adobe 4.1\n" if($opt_verbose);
		@filelist = @FILES_WIN40_ADOBE_4_1;
		}
	# If not, fall back to the one that came with MS-Windows 95.
	elsif($HAVE_WIN40_MS)
		{
		print STDERR "\tWin95 driver chosen: MS-Windows 95 (PSCRIPT.DRV 4.0)\n" if($opt_verbose);
		@filelist = @FILES_WIN40_MS;
		}
	else
		{
		print STDERR "No suitable Win95 driver found for \"$file\".\n";
		}

	if(scalar @filelist > 0)
		{
		my $driverpath = $filelist[0];
		my $helpfile = $filelist[1];
		unlink("$DRVDIR_WIN40/$mswin_name");
		link("$DRVDIR_WINPPD/$mswin_name", "$DRVDIR_WIN40/$mswin_name") || die $!;
		}
	}

	#=================================================================
	# Windows NT 4.0 driver
	#=================================================================
	{
	my @filelist = ();

	if($languagelevel > 1 && $HAVE_W32X86_2_ADOBE_5)
		{
		print STDERR "\tWinNT 4.0 driver chosen: Adobe 5.x\n" if($opt_verbose);
		@filelist = @FILES_W32X86_2_ADOBE_5;
		}
	elsif($HAVE_X32X86_2_MS)
		{
		print STDERR "\tWinNT 4.0 driver chosen: MS-Windows NT\n" if($opt_verbose);
		@filelist = @FILES_X32X86_2_MS;
		}
	else
		{
		print STDERR "No suitable WinNT 4.0 driver found for \"$file\".\n";
		}

	if(scalar @filelist > 0)
		{
		foreach (@filelist)
			{
			s#/#\\#g;
			$_ = "$DRVDIR_SHARE\\$W32X86\\$_";
			}
		my $driverpath = $filelist[0];
		my $configfile = $filelist[1];
		my $helpfile = $filelist[2];
		print DRIVERS "Windows NT x86:2:$nickname:$driverpath:$DRVDIR_SHARE\\$WINPPD\\$mswin_name:$configfile:$helpfile:NULL:RAW:", join(":", @filelist), "\n";
		}
	}

	#=================================================================
	# Windows NT 5.0 driver
	#=================================================================
	{
	my @filelist = ();

	if($languagelevel > 1 && $HAVE_W32X86_3_ADOBE_5)
		{
		print STDERR "\tWinNT 5.0 driver chosen: Adobe 5.x\n" if($opt_verbose);
		@filelist = @FILES_W32X86_3_ADOBE_5;
		}
	elsif($HAVE_W32X86_3_MS)
		{
		print STDERR "\tWinNT 5.0 driver chosen: MS-Windows NT\n" if($opt_verbose);
		@filelist = @FILES_W32X86_3_MS;
		}
	else
		{
		print STDERR "No suitable WinNT 5.0 driver found for \"$file\".\n";
		}

	if(scalar @filelist > 0)
		{
		foreach (@filelist)
			{
			s#/#\\#g;
			$_ = "$DRVDIR_SHARE\\$W32X86\\$_";
			}
		my $driverpath = shift @filelist;
		my $configfile = shift @filelist;
		my $helpfile = shift @filelist;
		print DRIVERS "Windows NT x86:3:$nickname:$driverpath:$DRVDIR_SHARE\\$WINPPD\\$mswin_name:$configfile:$helpfile:NULL:RAW:", join(":", @filelist), "\n";
		}
	}

	print STDERR "\n" if($opt_verbose);
	} # end of PPD file iteration

close(DRIVERS) || die $!;

print STDERR "Done.\n" if($opt_verbose);

exit(0);

