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
# Last modified 11 April 2002.
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

# This function search the INFOPATH.
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

# Print the CGI header.
print <<"EndHeader";
Content-Type: text/html

EndHeader

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

    my $cleaned_path = docs_open(DOC, $path);
    docs_last_modified(DOC);

    $/ = "";
    while(<DOC>)
	{
	print "<p>", html($_), "</p>\n";
	}

    close(DOC) || die $!;
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
