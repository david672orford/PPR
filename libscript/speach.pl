#
# mouse:~ppr/src/libppr/speach.pl
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

#
# This module contains routines needed to assemble spoken messages
# from recorded components.  It does not contain any routines for
# playing the resulting file.
#

# This should be defined before we are included:
defined($SHAREDIR) || die;

# Top level sound files directory.
my $SOUNDS_ROOT_DIR = "$SHAREDIR/speach";

# Voice to use when sound file doesn't exist in the selected voice:
my $FALLBACK_SOUNDS_DIR = "$SOUNDS_ROOT_DIR/male1";

# Directory with sounds for the current voice:
my $SOUNDS_DIR = $FALLBACK_SOUNDS_DIR;

# Sound to play when sound file not found:
my $MISSING_SOUND = "$SOUNDS_ROOT_DIR/silly_sounds/bounce.au";

# Have we initialized the random number generator yet?
my $srand_called = 0;

#
# This is used to test if the system administrator has installed
# the PPR sounds file distribution.
#
sub speach_soundfiles_installed
  {
  return (-r "$FALLBACK_SOUNDS_DIR/a.au");
  }

#
# Change the voice that is speaking.  Return TRUE if
# the requested voice exists, FALSE otherwise.
#
sub speach_set_voice
  {
  my $voice = shift;
  if(-d "$SOUNDS_ROOT_DIR/$voice")
    {
    $SOUNDS_DIR = "$SOUNDS_ROOT_DIR/$voice";
    return 1;
    }
  else
    {
    return 0;
    }
  }

#
# Test if a certain sound is available.
#
sub speach_sound_available
  {
  my $SOUND = shift;
  if(-r "$SOUNDS_DIR/$SOUND.au" || -r "$FALLBACK_SOUNDS_DIR/$SOUND.au")
  	{ return 1 }
  else
  	{ return 0 }
  }

#
# Play the indicated .au files.
#
sub speach_cat_au
  {
  my $handle = shift;
  my @filelist = @_;

  # Suppress spurios warnings
  my $header = "";
  my $data = "";

  # Do each file:
  my $infile;
  foreach $infile (@filelist)
    {
    if(!defined($infile))
    	{
	print STDERR "Can't play undef!\n" if($DEBUG > 0);
	$infile = $MISSING_SOUND;
    	}

    if(ref($infile) eq "ARRAY")
	{
	# Seed the random number generator if it has
	# not been seeded yet.
	if(! $srand_called)
	    {
	    srand();
	    $srand_called = 1;
	    }

	# Pick a random number between 0 and 99.
	my $r = int(rand(100));
	my $x;

	# Try each sound in turn, using it if the value is
	# within its range.
	for($x = 0; $x < $#{$infile}; $r-=$infile->[$x], $x+=2)
	    {
	    print STDERR "r = $r, sound = $infile->[$x]\n" if($DEBUG > 5);
	    last if($r < $infile->[$x]);
	    }

	# If we have run out of sound, don't play one.
	next if( ! ($x < $#{$infile}) );

	# Assign the selected sound to $infile so
	# it will get played.
	$infile = $infile->[$x + 1];
	}

    # Open this sound file.  If its name begins
    # with "*" then it is a silly sound.
    if($infile =~ /^\*/o)
	{
	my $file = substr($infile, 1);
	if( ! open(IN, "< $SOUNDS_ROOT_DIR/silly_sounds/$file.au") )
	    {
	    print STDERR "Missing sound: \"$infile\"\n" if($DEBUG > 0);
	    open(IN, "< $MISSING_SOUND") || die;
	    }
	}

    # If it doesn't begin with "*", look for it in
    # the directory for the selected voice.
    elsif( ! open(IN, "< $SOUNDS_DIR/$infile.au") )
	{
	# Try to fall back to another voice
	if( ! open(IN, "< $FALLBACK_SOUNDS_DIR/$infile.au") )
	    {
	    print STDERR "Missing sound: \"$infile\"\n" if($DEBUG > 0);
	    open(IN, "< $MISSING_SOUND") || die;
	    }
	}

    # Read the header
    if( read(IN, $header, 24) != 24 ) { die; }

    # Unpack the header
    my($magic, $hdr_size, $data_size, $encoding, $sample_rate, $channels) =
	unpack("a4NNNNN", $header);

    # Make sure this is a bit endian .au file.
    die if($magic ne ".snd");

    # Repack the header with extra bytes removed and
    # the data_size unspecified
    $data_size = ~0;
    if( ! $header_emmited )
      {
      print $handle pack(a4NNNNN, ".snd", 24, $data_size, $encoding, $sample_rate, $channels);
      $header_emmited = 1;
      }

    seek(IN, $hdr_size - 24, 1) || die;

    my $data = "";
    while( read(IN, $data, 4096) )
      { print $handle $data; }

    close(IN);
    }
  } # speach_cat()

#
# Spell out the indicated word by pushing the individual letters
# onto the indicated playlist.
#
sub speach_spellout
    {
    my($playlist, $word) = @_;
    my $letter;

    $word =~ tr/[A-Z]/[a-z]/;

    foreach $letter (split(//,$word))
	{
	push(@$playlist, $letter);
	}
    }

#
# Push the words for the indicated number onto the playlist.
#
sub speach_number_99
    {
    my($playlist, $number) = @_;
    my $tens;
    my $ones_place;

    if($number <= 20)
	{
	push(@$playlist, $number);
	}
    else
	{
	if(($tens = int($number/10)*10) > 0)
	    { push(@$playlist, $tens); }
	if(($ones_place = $number % 10) > 0)
	    { push(@$playlist, $ones_place); }
	}
    }

sub speach_number_999
    {
    my($playlist, $number) = @_;
    my $hundreds_place;

    if(($hundreds_place = int($number / 100)) > 0)
	{
	push(@$playlist, $hundreds_place);
	push(@$playlist, "hundred");
	}

    speach_number_99($playlist, ($number % 100) );
    }

sub speach_number_999999
    {
    my($playlist, $number) = @_;
    my $thousands;

    if(($thousands = int($number/1000)) > 0)
	{
	speach_number_999($playlist, $thousands);
	push(@$playlist, "thousand");
	}

    speach_number_999($playlist, ($number % 1000));
    }

sub speach_number_999999999
    {
    my($playlist, $number) = @_;
    my $millions;

    if(($millions = int($number/1000000)) > 0)
	{
	speach_number_999999($playlist, );
	push(@$playlist, "million");
	}

    speach_number_999999($playlist, ($number % 1000000));
    }

sub speach_number_999999999999
    {
    my($playlist, $number) = @_;
    my $thousand_millions;

    if(($thousand_millions = int($number/1000000000)) > 0)
	{
	speach_number_999999999($playlist, );
	push(@$playlist, "thousand million");
	}

    speach_number_999999999($playlist, ($number % 1000000000));
    }

sub speach_number
    {
    my($playlist, $number) = @_;

    if($number > 999999999999)
	{
	push(@$playlist, "number too big to pronounce");
	}
    else
	{
	speach_number_999999999999($playlist, $number);
	}
    }

#
# Push the words for the indicated time interval onto the playlist
#
sub speach_time_interval_division
    {
    my($playlist, $division, $n) = @_;

    if($n == 1)
	{
	push(@$playlist, "one $division");
	}
    elsif( $n > 0 )
	{
	#speach_number_99($playlist, $n);
	speach_number($playlist, $n);
	push(@$playlist, "${division}s");
	}
    }

sub speach_time_interval
    {
    my($playlist, $interval) = @_;

    my $seconds = $interval % 60;
    $interval = int($interval / 60);

    my $minutes = $interval % 60;
    $interval = int($interval / 60);

    my $hours = $interval % 24;
    $interval = int($interval / 24);

    my $days = $interval;

    speach_time_interval_division($playlist, "day", $days);
    speach_time_interval_division($playlist, "hour", $hours);
    speach_time_interval_division($playlist, "minute", $minutes);
    speach_time_interval_division($playlist, "second", $seconds);
    }

1;
