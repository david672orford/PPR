#! /usr/bin/perl
#
# misc/printer_list.perl
#

$CONFDIR = "/etc/ppr/printers";

format STDOUT_TOP =
Name	   Description												 PPD							 Address
----------------------------------------------------------------------------------------------------------------------------------------------------------
.

format STDOUT =
@<<<<<<<<<<@<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<@<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<@<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
$p,		   $comment,												 $ppdfile,						 $address
.

$= = 70;

opendir(DIR, $CONFDIR) || die;

foreach $p (sort(readdir(DIR)))
  {
  next if( $p =~ /^\./ );

  open(FILE, "<$CONFDIR/$p") || die;

  $comment = "";
  $interface = "";
  $address = "";
  $ppdfile = "";

  while(<FILE>)
	{
	if( $_ =~ /^Comment: +(.+)$/ )
	  { $comment = $1; }
	elsif ( $_ =~ /^Interface: +(.+)$/ )
	  { $interface = $1; }
	elsif( $_ =~ /^Address: +["]?([^"]+)["]?$/ )
	  { $address = $1; }
	elsif( $_ =~ /^PPDFile: +(.+)$/ )
	  { $ppdfile = $1; }
	}

  write;

  close(FILE);
  }

closedir(DIR);

exit(0);
