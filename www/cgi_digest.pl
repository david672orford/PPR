#
# mouse:~ppr/src/www/cgi_digest.pl
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
# Last modified 27 February 2004.
#

require "paths.ph";
defined($CONFDIR) || die;

# Temporary
$PWFILE = "$CONFDIR/htpasswd";
$SECRET = "jlkfjasdf8923sdklf";
$REALM = "printing";
$MAX_NONCE_AGE = 600;

# Hash a string using MD5 and return the hash in hexadecimal.
sub md5hex
	{
	my $string = shift;

	# This version uses the reference implementation from RSA.
	#require "MD5.pm";
	#my $md5 = new MD5;
	#$md5->reset;
	#$md5->add($string);
	#return unpack("H*", $md5->digest());

	# This version uses a pure Perl MD5 implementation.
	require "MD5pp.pm";
	return unpack("H*", MD5pp::Digest($string));
	}

# This function generates the hashed part of the server nonce.
sub digest_nonce_hash
	{
	my $nonce_time = shift;
	my $domain = shift;
	return md5hex("$nonce_time:$domain:$SECRET");
	}

# Create a nonce.
sub digest_nonce_create
	{
	my $domain = shift;
	my $nonce_time = time();
	#print STDERR "digest_nonce_create(): \$nonce_time=$nonce_time, \$domain=\"$domain\"\n";
	my $nonce_hash = digest_nonce_hash($nonce_time, $domain);
	return "$nonce_time:$nonce_hash";
	}

# Make sure a nonce is valid.
sub digest_nonce_validate
	{
	my $domain = shift;
	my $nonce = shift;

	$nonce =~ /^(\d+):(.+)$/ || die "Nonce \"$nonce\" is malformed.\n";
	my($nonce_time, $nonce_hash) = ($1, $2);

	my $correct_nonce_hash = digest_nonce_hash($nonce_time, $domain);
	#print STDERR "digest_nonce_validate(): \$nonce_time=$nonce_time, \$domain=\"$domain\", \$nonce_hash=\"$nonce_hash\", \$correct_nonce_hash=\"$correct_nonce_hash\"\n";
	die "Nonce hash is incorrect.\n" if($nonce_hash ne $correct_nonce_hash);

	my $time_now = time();
	#print STDERR "\$time_now=$time_now, \$nonce_time=$nonce_time\n";
	if($nonce_time > $time_now || $nonce_time < ($time_now - $MAX_NONCE_AGE))
		{
		return 0;
		}
	else
		{
		return 1;
		}
	}

# Find the user's entry (for the correct realm) in the private
# password file.
sub digest_getpw
	{
	my $sought_username = shift;
	#print STDERR "digest_getpw(\"$sought_username\")\n";

	my $answer = undef;

	open(PW, "<$PWFILE") || die "Can't open \"$PWFILE\", $!\n";
	while(<PW>)
		{
		chomp;
		my($username, $realm, $hash) = split(/:/);
		if($username eq $sought_username && $realm eq $REALM)
			{
			$answer = $hash;
			last;
			}
		}
	close(PW) || die;

	return $answer if(defined($answer));
	die "User \"$sought_username\" not listed in \"$PWFILE\".\n";
	}

1;
