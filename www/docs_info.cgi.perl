#! /usr/bin/perl -wT
#
# mouse:~ppr/src/www/docs_info.cgi.perl
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
# Last modified 18 April 2002.
#

#
# This program is a GNU Info documentation viewer.
#

use lib "?";
use PPR;
require "cgi_data.pl";
require "cgi_intl.pl";
require "docs_util.pl";

@INFOPATH = ("/usr/local/info", "/usr/info", "/usr/share/info");

# Prevent taint problems when running gzip and such.  See Programming 
# Perl 3rd edition, p. 565.
defined($ENV{PATH} = $PPR::SAFE_PATH) || die;
delete @ENV{qw(IFS CDPATH ENV BASH_ENV)};

# This function searches the INFOPATH for a specified document and returns
# the name of its file or undef if it can't find the document.
sub search_infopath
    {
    my $infopath = shift;
    my $name = shift;

    foreach my $dir (@{$infopath})
	{
	next if(! -d $dir);		# skip directories that don't exist

	foreach my $compext ("", ".gz", ".bz2")
	    {
	    my $filename = "$dir/$name.info$compext";
	    if(-f $filename)
		{
		return $filename;
		}
	    }
	}

    return undef;
    }

# Read POST and query variables.
&cgi_read_data();

# Hack for form searching.
if(defined(my $document = cgi_data_move("document", undef)))
    {
    require "cgi_redirect.pl";
    cgi_redirect("http://$ENV{SERVER_NAME}:$ENV{SERVER_PORT}$ENV{SCRIPT_NAME}/INFOPATH/$document");
    }

# Initialize the internationalization libraries and determine the
# content language and charset.
my ($charset, $content_language) = cgi_intl_init();

print "Content-Type: text/html;charset=$charset\n";

eval {
    my $path;

    # If the path begins with "/INFOPATH/", find it.
    if($ENV{PATH_INFO} =~ m#/INFOPATH/(.+)#)
	{
	my $doc = $1;
	$doc !~ m#/# || die "No subirectories within INFOPATH";
	$path = search_infopath(\@INFOPATH, $doc);
	defined($path) || die "Can't find $doc in INFOPATH";
	}

    # Otherwise, accept the path that the user supplies.
    else
	{
	$path = $ENV{PATH_INFO};
	}

    # Consume a list of files.  Files may be added to the end of the list
    # as we go.
    {
    my @paths = ($path);
    my $count = 0;
    my $dirname;
    my $dotext = "";
    local($/) = "";
    while(defined($path = shift @paths))
	{
	# Open the document
	my $cleaned_path = docs_open(DOC, $path);

	# We use the modification date of the first file to complete to 
	# CGI header.
	if($count++ == 0)
	    {
	    $dirname = $cleaned_path;
	    $dirname =~ s#/[^/]+$##;
	    if($cleaned_path =~ m#(\.[^/\.]+)$#)
		{
		$dotext = $1;
		}
	    docs_last_modified(DOC);
	    print "\n";
	    print "<html><head><title>", html($cleaned_path), "</title></head>\n<body><h1>", html($cleaned_path), "</h1>\n";
	    }

	my $state = "discard";
	while(<DOC>)
	    {
	    s/\s+$//;			# Delete trailing newlines, etc.

	     print STDERR ">>>", $_, "<<<\n";

	    if(/^\x1f/)			# This character marks the start of
		{			# non-text data
		if(/^\x1f\nFile:/s)	# Navigation point
		    {
		    if(/\bNode:\s*(\S+)/)
			{
			print "<a name=\"$1\">\n";
			}
		    $state = "normal";
		    next;
		    }

		if(/^\x1f\nIndirect:\n/s)		# include files
		    {
		    while(m/^([^:]+): +\d+$/mg)
			{
			push(@paths, "$dirname/$1$dotext");
			}
		    next;
		    }

		next;
		}

	    # We discard everthing before the first navigation point
	    next if($state eq "discard");

	    if($state eq "normal")
		{
		# Info format handles subheadings by underlining them
		# with characters such as "*", "=", "-", and ".".
		if(/^(.+)\n([\* ]+)\n/ && length($1) == length($2))
		    {
		    print "<h2>", html($1), "</h2>\n";
		    next;
		    }
		if(/^(.+)\n([= ]+)\n/ && length($1) == length($2))
		    {
		    print "<h3>", html($1), "</h3>\n";
		    next;
		    }
		if(/^(.+)\n([- ]+)\n/ && length($1) == length($2))
		    {
		    print "<h4>", html($1), "</h4>\n";
		    next;
		    }
		if(/^(.+)\n([\. ]+)\n/ && length($1) == length($2))
		    {
		    print "<h4>", html($1), "</h4>\n";
		    next;
		    }





		print "<p>", html($_), "</p>\n";
		next;
		}

	    print "<!-- ", html($_), " -->\n";
	    }

	close(DOC) || die $!;
	}
    }

    };
if($@)
    {
    my $message = $@;
    print "<p>", H_("Error: "), html($message), "</p>\n";
    }

print <<"EndOfTail";
</body>
</html>
EndOfTail

exit 0;
