#
# mouse:~ppr/src/www/cgi_user_agent.pl
# Copyright 1995--2003, Trinity College Computing Center.
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
# Last modified 17 December 2003.
#

sub cgi_user_agent
	{
	my %facts;
	$facts{mozilla_version} = 0.0;
	$facts{opera_version} = 0.0;
	$facts{links_version} = 0.0;
	$facts{gecko_version} = 0.0;
	$facts{css_fixed} = 0;				# Does CSS fixed positioning work?
	$facts{css_hover} = 0;				# Does CSS :hover work on any object?
	$facts{css_dom} = 0;				# Can CSS attributes be manipulated using the W3C DOM?
	$facts{button} = 0;					# Do HTML 4.0 <button> tags work?
	$facts{images} = 0;					# can display images?

	if(defined(my $ua = $ENV{HTTP_USER_AGENT}))
		{
		print STDERR "\$ua=\"$ua\"\n";
		if($ua =~ /^Mozilla\/(\d+\.\d+) \(([^\)]+)\)/)
			{
			$facts{mozilla_version} = $1;
			my $details = $2;
			foreach my $i (split(/;\s+/, $details))
				{
				if($i =~ /^rv:(\d+\.\d+)$/)
					{
					$facts{gecko_version} = $1;
					}
				}
			if($ua =~ / Opera (\d+\.\d+)/)
				{ $facts{opera_version} = $1 }
			}

		elsif($ua =~ /^Opera\/(\d+\.\d+)/)
			{ $facts{opera_version} = $1 }

		elsif($ua =~ /^Links \((\d+\.\d+)/)
			{
			$facts{links_version} = $1;
			}
		}

	# If this browser claims compatility with Mozilla level five or greater
	# or it really is Mozilla 1.0 or later
	# or it is Opera 7 or greater, assume it supports all sorts of neat stuff.
	if($facts{mozilla_version} >= 5.0 || $facts{gecko_version} >= 1.0 || $facts{opera_version} >= 7.0)
		{
		$facts{css_fixed} = 1;
		$facts{css_hover} = 1;
		$facts{css_dom} = 1;
		$facts{button} = 1;
		}

	# If it claims any level of Mozilla compatibility, it can probably do images.
	if($facts{mozilla_version} > 0.0 || $facts{opera_version} > 0.0)
		{
		$facts{images} = 1;
		}

	return \%facts;
	}

1;
