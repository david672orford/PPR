#! /usr/bin/perl -w
#
# mouse:~ppr/src/misc/ppd2win95drv.perl
# Copyright 1995--2002, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is" without
# express or implied warranty.
#
# Last modified 1 May 2002.
#

#
# This script is used to prepare a Samba shared directory for Win95 driver
# installations.  This script should run with Perl 4 or 5.
#

$CONFDIR="?";
$SHAREDIR="?";
$VAR_SPOOL_PPR="?";

# This is the directory which contains the PPR printer configuration files.
$PRINTERSDIR = "$CONFDIR/printers";

# This is the directory which contains PPR's collection of PPD files.
$PPD_LIB_DIR = "$SHAREDIR/PPDFiles";

# These are the directories from which clients download drivers.
$DRVDIR = "$VAR_SPOOL_PPR/drivers";
$DRVDIR_SHARE = "\\pprdrvs\$";
$W32X86 = "W32X86";				# Windows NT x86
$WIN40 = "WIN40";				# Windows 4.0 (95)
$WINPPD = "WINPPD";				# PPD files in MS-DOS text format
$DRVDIR_W32X86 = "$DRVDIR/$W32X86";
$DRVDIR_WIN40 = "$DRVDIR/$WIN40";
$DRVDIR_WINPPD = "$DRVDIR/$WINPPD";

# We use this when copying in new PPD files.
$TEMP_PPD_FILE = "$DRVDIR_WINPPD/NEW";

# List of files for the MS-Windows 4.0 (95) PostScript driver.  This driver
# can drive both level 1 and level 2 PostScript printers.
@FILES_WIN40_MS = qw(PSCRIPT.DRV PSCRIPT.HLP FONTS.MFM ICONLIB.DLL PSMON.DLL PSCRIPT.INI);

# List of files for the Adobe version 4.1 PostScript driver for Windows 95.
# This driver can drive both level 1 and level 2 PostScript printers.
@FILES_WIN40_ADOBE_4_1 = qw(ADBEPS41.DRV ADOBEPS4.HLP FONTS.MFM ICONLIB.DLL PSMON.DLL);

# List of files for the Adobe version 4.2 PostScript driver.  This driver
# can drive level 2 but not level 1 PostScript printers.
@FILES_WIN40_ADOBE_4_2 = qw(ADOBEPS4.DRV ADOBEPS4.HLP ADFONTS.MFM ICONLIB.DLL PSMON.DLL PSCRIPT.INI);

# List of files for the Adobe version 5.x PostScript driver for Windows NT 
@FILES_W32X86_ADOBE_5 = qw(2/ADOBEPS5.DLL 2/ADOBEPSU.DLL 2/ADOBEPSU.HLP 2/ADOBEPS5.NTF);

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

#
# Create the output directory if it doesn't already exist.
#
mkdir($DRVDIR_WIN40, 0755);	# Win95
mkdir($DRVDIR_W32X86, 0755);	# WinNT
mkdir($DRVDIR_WINPPD, 0755);	# PPD Files

#
# Convert all of the filenames in the driver directories to upper case.
#
foreach my $dir ($DRVDIR_WIN40, "$DRVDIR_W32X86/2")
    {
    print STDERR "Changing filenames in \"$dir\" to upper case...\n" if($opt_verbose);
    opendir(DIR, $dir) || die "Can't open directory \"$dir\", $!\n";
    while(defined($file = readdir(DIR)))
	{
	next if($file =~ /^\./);
	next if($file eq "printers.def");

	$upper = $file;
	$upper =~ tr/[a-z]/[A-Z]/;
	if($upper ne $file)
	    {
	    print "\t$file -> $upper\n" if($opt_verbose);
	    if(!rename("$dir/$file", "$dir/$upper"))
		{
		die "Can't rename \"$dir/$file\" to \"$dir/$upper\", $!\n";
		}
	    }
	}
    closedir(DIR);
    print "Done.\n\n" if($opt_verbose);
    }

#
# This function is used to figure out which Win95 drivers are
# installed.  It is passed a list of files.  If all of them
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
	if(!$result) { print STDERR "not " }
	print STDERR "present.\n\n";
    	}
    return $result;
    }

$HAVE_WIN40_MS = allfound("Win95 Microsoft driver", $DRVDIR_WIN40, @FILES_WIN40_MS);
$HAVE_WIN40_ADOBE_4_1 = allfound("Win95 Adobe driver 4.1", $DRVDIR_WIN40, @FILES_WIN40_ADOBE_4_1);
$HAVE_WIN40_ADOBE_4_2 = allfound("Win95 Adobe driver 4.2x", $DRVDIR_WIN40, @FILES_WIN40_ADOBE_4_2);
$HAVE_W32X86_ADOBE_5 = allfound("WinNT Adobe driver 5.x", $DRVDIR_W32X86, @FILES_W32X86_ADOBE_5);

#
# Convert the PPD files and print the driver descriptions.
#
if($opt_verbose)
    {
    print STDERR "Scanning \"$PPD_LIB_DIR\", converting PPD files...\n";
    }

# List of PPD files to copy into the driver distribution share.
%ppd_files = ();

# First include all of the PPD files distributed with PPR.
opendir(DIR, $PPD_LIB_DIR) || die "Can't open directory \"$PPD_LIB_DIR\", $!\n";
while(defined($file = readdir(DIR)))
    {
    next if($file =~ /\./);	# skip hidden files
    next if($file =~ /~$/);	# skip backup files
    next if($file =~ /\.bak$/i);

    $ppd_files{"$PPD_LIB_DIR/$file"} = '';
    }
closedir(DIR) || die;

# Now, include the files used by the printers.  Generally these
# files will be some of the ones found in the step above, but
# some printers may be using PPD files outside PPR's PPDFiles
# directory.
opendir(DIR, $PRINTERSDIR) || die "Can't open directory \"$PRINTERSDIR\", $!\n";
while(defined($file = readdir(DIR)))
    {
    next if($file =~ /\./);	# skip hidden files
    next if($file =~ /~$/);	# skip backup files
    next if($file =~ /\.bak$/i);

    open(FILE, "<$PRINTERSDIR/$file") || die "Can't open \"$PRINTERSDIR/$file\", $!\n";
    while(<FILE>)
	{
	if(/^PPDFile:\s+(.+?)\s*$/)
	    {
	    $_ = $1;
	    if(/^[^\/]/)
	    	{ $_ = "$PPD_LIB_DIR/$_" }
	    $ppd_files{$_} = '';
            last;
	    }
	}
    close(FILE);
    }
closedir(DIR) || die;

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
	# ignore it if it is not in the outermost file.  This
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

    # If we can use the lastest Adobe driver, do so.
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
	foreach (@filelist)
	    {
	    s#/#\\#g;
	    $_ = "$DRVDIR_SHARE\\$WIN40\\$_";
	    }
	my $driverpath = $filelist[0];
	my $helpfile = $filelist[1];
	print "Windows 4.0:0:$nickname:$driverpath:$DRVDIR_SHARE\\$WINPPD\\$mswin_name:NULL:$helpfile:NULL:RAW:", join(":", @filelist), "\n";
	}
    }

    #=================================================================
    # Windows NT driver
    #=================================================================
    {
    my @filelist = ();

    if($languagelevel > 1 && $HAVE_W32X86_ADOBE_5)
	{
	print STDERR "\tWinNT driver chosen: Adobe 5.x\n" if($opt_verbose);
	@filelist = @FILES_W32X86_ADOBE_5;
	}
    else
	{
	print STDERR "No suitable WinNT driver found for \"$file\".\n";
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
	print "Windows NT x86:2:$nickname:$driverpath:$DRVDIR_SHARE\\$WINPPD\\$mswin_name:$configfile:$helpfile:NULL:RAW:", join(":", @filelist), "\n";
	}
    }

    print "\n" if($opt_verbose);
    } # end of PPD file iteration

print "Done.\n" if($opt_verbose);

exit(0);

