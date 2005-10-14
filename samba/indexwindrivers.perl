#! @PERL_PATH@ -w
#
# mouse:~ppr/src/samba/indexwindrivers.perl
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
# Last modified 12 October 2005.
#

#
# This script is used to prepare a Samba shared directory for Win95 driver
# installations.  This script should run with Perl 4 or 5.
#

use lib "@PERL_LIBDIR@";
require "readppd.pl";

$CONFDIR="@CONFDIR@";
$LIBDIR="@LIBDIR@";
$VAR_SPOOL_PPR="@VAR_SPOOL_PPR@";

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

$SAMBAPRINT = "$LIBDIR/sambaprint";

$opt_verbose = 1;

if(! -x $SAMBAPRINT)
	{
	print STDERR "No Samba TDB support.\n";
	exit 1;
	}

#
# Create the output directory if it doesn't already exist.
#
mkdir($DRVDIR_WIN40, 0755);			# Win95/98/ME
mkdir($DRVDIR_W32X86, 0755);		# WinNT
mkdir("$DRVDIR_W32X86/2", 0755);
mkdir("$DRVDIR_W32X86/3", 0755);	# WinNT 5.0
mkdir($DRVDIR_WINPPD, 0755);		# PPD Files

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

# Open the index created by "ppr-index ppds".
open(INDEX, $PPD_INDEX) || die "Can't open index file \"$PPD_INDEX\", $!\n";

# Open sambaprint to accept the driver definitions for Samba.
open(DRIVERS, "| $SAMBAPRINT drivers import") || die $!;

# For each PPD file, convert it to MS-DOS line termination and store it in the
# WINPPD subdirectory of the driver distribution share under its MS-DOS file
# name.
while(defined(my $index_line = <INDEX>))
	{
	next if($index_line =~ /^#/);		# skip comments (which occur in header)
	my($file) = (split(/:/, $index_line))[1];
	defined $file || die "bad index";
	-f $file || die "index is out of date";

	print STDERR "Processing \"$file\":\n" if($opt_verbose);

	open(OUT, ">$TEMP_PPD_FILE") || die "Can't create \"$TEMP_PPD_FILE\", $!\n";
	undef $mswin_name;
	undef $nickname;
	undef $languagelevel;

	ppd_open($file);

	while(defined(my $line = ppd_readline()))
		{
		# Copy the line to the output
		print OUT "$line\r\n";

		# If this is the first PC file name line take it, but we
		# ignore it if it is not in the outermost file.	 This
		# rule isn't in the PPD spec, but it seems right.
		if(! defined($mswin_name) && $line =~ /^\*PCFileName:\s+"([^"]+)"/ && ppd_level() == 1)
			{
			$mswin_name = $1;
			$mswin_name =~ tr/a-z/A-Z/;
			}

		# Take first "*NickName:" or "*ShortNickName:" line.  The PPD
		# spec says that the "*ShortNickName:" must be first if it
		# is present.
		if(! defined($nickname) && $line =~ /^\*(Short)?NickName:\s+"([^"]+)"/)
			{
			$nickname = $2;
			}

		# Take first "*LanguageLevel:" line.
		if(! defined($languagelevel) && $line =~ /^\*LanguageLevel:\s+"([0-9]+)"/)
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
close(INDEX) || die $!;

print STDERR "Done.\n" if($opt_verbose);

exit(0);

