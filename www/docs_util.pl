#
# mouse:~ppr/src/www/docs_util.pl
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
# Last modified 5 April 2003.
#
  
require "cgi_data.pl";
require "cgi_time.pl";

#
# This subroutine is used to open a documentation file.	 It is passed the name 
# of a file handle and the name of a file.	The file name probably cames
# from $ENV{PATH_INFO}.	 It opens the file and returns a cleaned up version
# of the name.	It will refuse to open the file if it isn't in a recognized
# documentation directory.	If the file is compressed, it runs an appropriate
# decompression program on it and returns a handle connected to the output
# of the decompression program.
#
sub docs_open
	{
	my $handle = shift;
	my $path = shift;

	defined($path) || die _("This script was invoked without any trailing path.  In other words,\n"
						. "it should have been treated as if it were a subdirectory.\n");

	# Clean up the path.  Note that PPR's own ppr-httpd already
	# does some of this.
	$path =~ s#//##g;					# remove sillyness
	$path =~ s#/\./#/#g;				# convert "/./" to "/"
	$path =~ s#/[^/]+/\.\./#/#g;		# convert "*/../" to "/"


	# That's good enough.  Let's detaint it.
	$path =~ m#^(.*)#;
	$path = $1;

	# Accept only well-known documentation directories.
	if(!($path =~ m#^/usr/doc/#
		|| $path =~ m#^/usr/share/doc/#
		|| $path =~ m#^/usr/man/#
		|| $path =~ m#^/usr/share/man/#
		|| $path =~ m#^/usr/X11R6/man/#
		|| $path =~ m#^/usr/local/man/#
		|| $path =~ m#^/usr/info/#
		|| $path =~ m#^/usr/share/info/#
		|| $path =~ m#^/usr/local/info/#))
		{
		die sprintf(_("The file \"%s\" is not within a recognized document directory.\n"), $path);
		}

	# Open the input file.	This is tricky because it might be compressed.
	# We also have to be very careful not to pass anything tainted to a shell.
	if($path =~ /\.gz$/)
		{
		require 'cgi_run.pl';
		opencmd($handle, "gunzip", "-c", $path) || die $!;
		}
	elsif($path =~ /\.bz2$/)
		{
		require 'cgi_run.pl';
		opencmd($handle, "bunzip2", "-c", $path) || die $!;
		}
	else
		{
		require Fcntl;
		sysopen($handle, $path, &Fcntl::O_RDONLY) || die "Can't open \"$path\", $!";
		}

	return $path;
	}

#
# This subroutine tries to guess a MIME type from a file name.	If it
# is stumped, it returns "text/plain".
#
sub docs_guess_mime
	{
	my $path = shift;
	return "image/jpeg" if($path =~ /\.jpe?g$/);
	return "image/gif" if($path =~ /\.gif$/);
	return "text/html" if($path =~ /\.html?$/);
	return "application/x-troff-man" if($path =~ /\.[1-8][a-z]?(\.((gz)|(bz2)))?$/);
	return "application/x-info" if($path =~ /\.info(\.((gz)|(bz2)))?$/);
	return "text/plain";
	}

# If passed an open file handle (or file name I suppose), this function
# will print an HTTP "Last-Modified:" for the file.
sub docs_last_modified
	{
	my $handle = shift;
	my($mtime) = (stat($handle))[9];
	if(defined($mtime))
		{
		print "Last-Modified: ", cgi_time_format($mtime), "\n";
		}
	}

1;
