#
# mouse:~ppr/src/libscript/speach_play.pl
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
# Last modified 16 November 2000.
#

# We need these:
defined($TEMPDIR) || die;
defined($VAR_SPOOL_PPR) || die;

# This variable holds the SMB server name of this computer.	 If you
# leave it undefined here, it will be defined later as the part of
# the "uname -n" output before the first dot.  If you have Samba
# set up to user a different name, you will have to define it here.
my $THIS_SERVER = undef;

# The PPR client spooling area:
my $CLISPOOL_UNIX = "$VAR_SPOOL_PPR/pprclipr";
my $CLISPOOL_LANMAN = "\\\\$THIS_SERVER\\pprclipr";

#==============================================================
# Play the sound listed sounds on the device indicated by
# the address.
#==============================================================
sub speach_play_many
  {
  my $address = shift;

  if($address =~ /^\/dev\//)
		{ speach_play_many_local($address, @_) }
  else
		{ speach_play_many_smb($address, @_) }
  }

#==============================================================
# Direct the remote computer to play an audio file which we
# have deposited in the client spooling area.  This function
# is passed the LANMAN share name of the file.
#==============================================================
sub speach_play_many_smb
  {
  my ($address, @soundlist) = @_;

  # Pull in the routines for connecting to PPR popup.
  require 'pprpopup.pl';

  if(!defined($THIS_SERVER))
	{
	$THIS_SERVER = `uname -n`;
	chomp $THIS_SERVER;
	}

  # Temporary file to hold output.
  my $temp_au = $address;
  $temp_au =~ s/[^a-z0-9]/_/ig;			# slashes and colons would be poision
  $temp_au .= "-$$.au";

  print "Playing message on \"$address\"\n" if($DEBUG > 1);

  # Make a connexion to the remote machine:
  if(!open_connexion(CON, $address))
	{
	print "Connexion failed: $!\n" if($DEBUG > 1);
	return 0;
	}
  CON->autoflush(1);

  # Open a temporary file to hold it and concatenate the sounds into it.
  open(OUT, "> $CLISPOOL_UNIX/$temp_au") || die;
  speach_cat_au(OUT, @soundlist);
  close(OUT);

  # Tell the other end to expect a file of a certain size.
  print CON "SMBPLAY $file\n";

  my $result = <CON>;
  print "result = $result\n" if($DEBUG > 1);

  close(CON);

  unlink("$CLISPOOL_UNIX/$temp_au") || die;

  return 1;
  }

#==============================================================
# Play the indicated file on the server's own sound hardware.
#==============================================================
sub speach_play_many_local
  {
  my $address = shift;
  my @soundlist = @_;

  print "Playing message on device \"$address\"\n" if($DEBUG > 1);

  # Do a write access check on the audio device.
  (-w $address) || die "Can't write to device \"$address\"";

  # Temporary file to hold output.
  my $temp_au = $address;
  $temp_au =~ s#^/dev/##;				# don't make it too complicated!
  $temp_au =~ s/[^a-z0-9]/_/ig;			# slashes and colons would be poision
  $temp_au = "$TEMPDIR/ppr-${temp_au}-$$.au";

  # Open a temporary file to hold it and concatenate the sounds files into it.
  open(OUT, ">$temp_au") || die "Can't open \"$temp_au\" for write: $!";
  speach_cat_au(OUT, @soundlist);
  close(OUT);

  # RedHat Linux with Sox:
  if(-x "/usr/bin/play")
		{
		system("/usr/bin/play $temp_au");
		}

  # Solaris:
  elsif(-x "/usr/bin/audioplay")
		{
		system("/usr/bin/audioplay -d $address $temp_au");
		}

  # DEC OSF/1:
  elsif(-x "/usr/bin/mme/audioplay")
		{
		system("/usr/bin/mme/audioplay -filename $temp_au");
		}

  # Failure
  else
		{
		die "No local audio play program found!";
		}

  unlink($temp_au) || die;
  }

1;
