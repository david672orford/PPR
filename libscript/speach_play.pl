#
# mouse:~ppr/src/libscript/speach_play.pl
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
# Last modified 29 March 2005.
#

use PPR;

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
	my $temp_au  = `$PPR::LIBDIR/mkstemp "$PPR::TEMPDIR/ppr-speach_play-XXXXXX"`;
	chomp $temp_au;

	# Open a temporary file to hold it and concatenate the sounds files into it.
	open(OUT, ">$temp_au") || die "Can't open \"$temp_au\" for write: $!";
	speach_cat_au(OUT, @soundlist);
	close(OUT);

	# Alsa Player
	#if(-x "/usr/bin/aplay")
	#	{
	#	system("/usr/bin/aplay", "--device", $address, $temp_au);
	#	}

	# RedHat Linux with Sox:
	if(-x "/usr/bin/play")
		{
		system("/usr/bin/play", "-d", $address, $temp_au);
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

	unlink($temp_au) || die "Can't delete $temp_au: $!";
	}

1;
