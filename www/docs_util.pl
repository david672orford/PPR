#
# mouse:~ppr/src/www/docs_util.pl
# Copyright 1995--2001, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appears in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is"
# without express or implied warranty.
#
# Last modified 24 April 2001. 
#
  
require "cgi_data.pl";
require "cgi_time.pl";

#
# This subroutine is used to open a documentation file.  It is passed the name 
# of a file handle and the name of a file.  The file name problably cames
# from $ENV{PATH_INFO}.  It opens the file and returns a cleaned up version
# of the name.  It will refuse to open the file if it isn't in a recognized
# documentation directory.  If the file is compressed, it runs an appropriate
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
    $path =~ s#//##g;			# double slashes
    $path =~ s#/\./#/#g;		# single dots
    $path =~ s#/[^/]+/\.\./#/#g;	# double dots

    # Accept only well-known documentation directories.
    if(!($path =~ m#^/usr/doc/#
	|| $path =~ m#^/usr/man/#
	|| $path =~ m#^/usr/info/#
	|| $path =~ m#^/usr/share/doc/#
	|| $path =~ m#^/usr/share/man/#
	|| $path =~ m#^/usr/X11R6/man/#
	|| $path =~ m#^/usr/local/man/#
	|| $path =~ m#^/usr/local/info/#))
	{
	die sprintf(_("The file \"%s\" is not within a recognized document directory.\n"), $path);
	}

    # Open the input file.  This is tricky because it might be compressed.
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
	sysopen($handle, $path, O_RDONLY) || die $!;
	}

    return $path;
    }

#
# This subroutine tries to guess a MIME type from a file name.  If it
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
