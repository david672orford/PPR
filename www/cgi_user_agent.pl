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
# Last modified 23 December 2003.
#

sub cgi_user_agent
	{
	$mozilla_version = 0.0;
	$opera_version = 0.0;
	$gecko_version = 0.0;
	$msie_version = 0.0;
	
	my %facts;
	$facts{css} = 0;					# Very basic CSS support?
	$facts{css_fixed} = 0;				# Does CSS fixed positioning work?
	$facts{css_hover} = 0;				# Does CSS :hover work on any object?
	$facts{css_dom} = 0;				# Can CSS attributes be manipulated using the W3C DOM?
	$facts{button} = 0;					# Do HTML 4.0 <button> tags work?
	$facts{images} = 0;					# can display images?

	if(defined(my $ua = $ENV{HTTP_USER_AGENT}))
		{
		#print STDERR "\$ua=\"$ua\"\n";

		# Many graphical browsers, such as Internet Explorer, claim to be
		# Mozilla (Netscape).
		if($ua =~ /^Mozilla\/(\d+\.\d+) \(([^\)]+)\)/)
			{
			$mozilla_version = $1;
			my $details = $2;
			
			$facts{images} = 1;

			foreach my $i (split(/;\s+/, $details))
				{
				if($i =~ /^rv:(\d+\.\d+)$/)
					{
					$gecko_version = $1;
					}
				if($i =~ /^MSIE ([\d\.]+)$/)
					{
					$msie_version = $1;
					}
				}

			# Opera claiming to be IE will put its own name and version
			# number after the list of attributes in parenthesis.
			if($ua =~ / Opera (\S+)/)
				{ $opera_version = $1 }
			}

		elsif($ua =~ m#^Opera/(\S+)#)
			{
			$opera_version = $1;
			$facts{images} = 1;
			}

		elsif($ua =~ m#^Links \(([^\)]+)\)#)
			{
			my $details = $1;
			foreach my $i (split(/;\s+/, $details))
				{
				if($i eq "fb" || $i eq "x")		# framebuffer or X-Windows
					{ $facts{images} = 1 }
				}
			}

		elsif($ua =~ m#^Dillo/(\S+)#)
			{
			$facts{images} = 1;
			}

		#elsif($ua =~ m#^Lynx/(\S+)#)
		#	{
		#	}
		}

	# If this browser claims compatility with Mozilla level five or greater
	# or it really is Mozilla 1.0 or later
	# or it is Opera 7 or greater, assume it supports all sorts of neat stuff.
	if($mozilla_version >= 5.0 || $gecko_version >= 1.0 || $opera_version >= 7.0)
		{
		$facts{css} = 1;
		$facts{css_fixed} = 1;
		$facts{css_hover} = 1;
		$facts{css_dom} = 1;
		$facts{button} = 1;
		}

	# If the browser claims to be Microsoft Internet Explorer version 4.0 or
	# later, assume it has at least very basic CSS support with DOM bindings.
	if($msie_version >= 4.0)
		{
		$facts{css} = 1;
		$facts{css_dom} = 1;
		}

	return \%facts;
	}

1;
