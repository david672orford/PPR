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
	    return $filename if(-f $filename);
	    }
	}

    return undef;
    }

# Write the CGI header and the top of the document.
sub do_header
    {
    my($charset, $content_language, $filehandle, $title) = @_;
    print "Content-Type: text/html;charset=$charset\n";
    docs_last_modified($filehandle);
    print "\n";
    print <<"EndHead";
<html>
<head><title>${\html($title)}</title>
<style>
.error { color: green }
P.navigation
	{
	background-color: #FFE0E0;
	color: black;
	padding: 2mm;
	border: thin solid black;
	}
</style>
</head>
<body>
EndHead
    }

# Convert a string to HTML and change references to links.
sub linkize
    {
    my $text = html(shift);
    $text =~ s/\*([Nn]ote)\s+([^:]+)::/linkize_helper($1, $2)/ge;
    return $text;
    }
sub linkize_helper
    {
    my $note = shift;
    my $name = shift;
    return "*$note <a href=\"${info_to_url($name)}\">$name</a>::";
    }

# Preserve line breaks
sub linebreak
    {
    my $text = shift;
    $text =~ s/\n/<br>\n/g;
    return $text;
    }

# Convert an Info document specification to a URL.
sub info_to_url
    {
    my $info = shift;
    $info =~ s/\s+/ /g;
    ($info =~ /^(\((.+)\))?(.*)$/) || die "\$info = \"$info\"";
    my($doc, $fragment) = ($2, $3);
    $doc = "" if(!defined $doc);
    if(defined $fragment && $fragment ne "")
	{
	$fragment =~ s/ /_/g;
	$doc .= "#$fragment";
	}
    return $doc;
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

my $file_count = 0;
eval {
    my @paths;
    my $dirnode = 0;

    defined($ENV{PATH_INFO}) || die "No PATH_INFO!\n"; 

    # If the path begins with "/INFOPATH/", find it.
    if($ENV{PATH_INFO} =~ m#/INFOPATH/(.+)#)
	{
	my $doc = $1;
	$doc !~ m#/# || die "No subirectories within INFOPATH";
	if($doc eq "dir")
	    {
	    @paths = ();
	    foreach my $dir (@INFOPATH)
		{
		my $path = "$dir/dir";
		push(@paths, $path) if(-f $path);
		}
	    (scalar @paths > 0) || die "No directories found!\n";
	    $dirnode = 1;
	    }
	else
	    {
	    my $path = search_infopath(\@INFOPATH, $doc);
	    defined($path) || die "Can't find $doc in INFOPATH";
	    @paths = ($path);
	    }
	}

    # Otherwise, accept the path that the user supplies.
    else
	{
	@paths = ($ENV{PATH_INFO});
	}

    # Consume a list of files.  Files may be added to the end of the list
    # as we go.
    {
    my $dirname;
    my $dotext = "";
    local($/) = "";
    while(defined(my $path = shift @paths))
	{
	# Open the document
	my $cleaned_path = docs_open(DOC, $path);

	# We use the modification date of the first file to complete to 
	# CGI header.
	if($file_count == 0)
	    {
	    $dirname = $cleaned_path;
	    $dirname =~ s#/[^/]+$##;
	    if($cleaned_path =~ m#(\.[^/\.]+)$#)
		{
		if($1 ne ".info")
		    {
		    $dotext = $1;
		    }
		}
	    do_header($charset, $content_language, DOC, $cleaned_path);
	    $file_count++;
	    }

	print "<h1>", html($cleaned_path), "</h1>\n";

	my $state = "discard";
	while(<DOC>)
	    {
	    s/\s+$//;			# Delete trailing newlines, etc.

	    #print STDERR ">>>", $_, "<<<\n";

	    if(/^\x1f/)			# This character marks the start of
		{			# non-text data
		if(/^\x1f\nFile:/s)	# Navigation point
		    {
		    print "<p class=\"navigation\">";
		    if(/\bNode:\s*([^,]+)/)
			{
			my $name = $1;
			my $name_escaped = $name;
			$name_escaped =~ s/\s+/_/g;
			print "<a name=\"$name_escaped\">\n";
			print "Node: <a href=\"${\info_to_url($name)}\">", html($name), "</a> ";
			}
		    if(/\bNext:\s*([^,]+)/)
			{
			my $name = $1;
			print "Next: <a href=\"${\info_to_url($name)}\">", html($name), "</a> ";
			}
		    if(/\bPrev:\s*([^,]+)/)
			{
			my $name = $1;
			print "Prev: <a href=\"${\info_to_url($name)}\">", html($name), "</a> ";
			}
		    if(/\bUp:\s*([^,]+)/)
			{
			my $name = $1;
			print "Up: <a href=\"${\info_to_url($name)}\">", html($name), "</a> ";
			}
		    print "</p>\n";
		    $state = "normal" if($state eq "discard");
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

	    # dir nodes need this.
	    if($dirnode && /^\* Menu:/)
		{
		$state = "dirmenu";
		next;
		}

	    #
	    # This is for the dir nodes too.
	    #
	    if($state eq "dirmenu")
		{
		s/\n[ \t]+/ /g;			# unwrap long lines
		my @lines = split(/\n/);
		my $title = shift @lines;
		print "<h2>", html($title), "</h2>\n";
		print "<table border=1 cellspacing=0 cellpadding=3 width="100%">\n";
		print "<tr><th>Subject</th><th>Document</th><th>Section</th><th>Description</th></tr>\n";
		foreach (@lines)
		    {
		    if(/^\* ([^:]+): \((.+)\)(.*)\.\s+(.+)\.?$/)
			{
			my($subject, $filename, $section, $description) = ($1, $2, $3, $4);
			my $escaped_section = $section;
			$escaped_section =~ s/\s+/_/g;
			print "<tr>";
			print "<td><a href=\"$filename#$escaped_section\">", html($subject), "</a></td>";
			print "<td>", html($filename), "</td>";
			print "<td>", html_nb($section), "</td>";
			print "<td>", html($description), "</td>";
			print "</tr>\n";
			}
		    else
			{
			print "<p class=\"error\">", html($_), "</p>\n";
			}
		    }		
		print "</table>\n";
		next;
		}

	    #
	    # Dictionary list:
	    #
	    # 'r'
	    #      the permission the USERS have to read the file
	    #
	    # 'a'
	    # 'b'
	    #      the definition
	    #
	    #  - Option: -x
	    #      the definition of the -x option
	    #
	    if($state eq "normal" && /^((( - )?\S[^\n]*\n)+)     (.+)$/s)
		{
		print "<dl>\n";
		$state = "dl";
		}
	    if($state eq "dl")
		{
		# Dictionary list
		if(/^((( - )?\S[^\n]*\n)+)     (.+)$/s)
		    {
		    my($dt, $dd) = ($1, $4);
		    chomp $dt;
		    print "<dt>", linebreak(linkize($dt)), "</dt><dd>", linkize($dd), "</dd>\n";
		    next;
		    }
		else
		    {
		    print "</dl>\n";
		    $state = "normal";
		    }
		}

	    #
	    # Unordered list:
	    #
	    #    * item one
	    #    * item two
	    #
	    if($state eq "normal" && /^ +\*/)
		{
		print "<ul>\n";
		$state = "ul";
		}
	   if($state eq "ul")
		{
		if(/^ +\* (.+)$/s)
		    {
		    my $text = $1;
		    print "<li>", linkize($text), "\n";
		    next;
		    }
		else
		    {
		    print "</ul>\n";
		    $state = "normal";
		    }
		}

	    #
	    # Ordered list:
	    #
	    if($state eq "normal" && /^ +\d/)
		{
		print "<ol>\n";
		$state = "ol";
		}
	   if($state eq "ol")
		{
		if(/^ +\d+\. (.+)$/s)
		    {
		    my $text = $1;
		    print "<li>", linkize($text), "\n";
		    next;
		    }
		else
		    {
		    print "</ol>\n";
		    $state = "normal";
		    }
		}

	    if($state eq "normal")
		{
		# Info format handles subheadings by underlining them
		# with characters such as "*", "=", "-", and ".".
		if(/^(.+)\n([\* ]+)$/ && length($1) == length($2))
		    {
		    print "<h2>", html($1), "</h2>\n";
		    next;
		    }
		if(/^(.+)\n([= ]+)$/ && length($1) == length($2))
		    {
		    print "<h3>", html($1), "</h3>\n";
		    next;
		    }
		if(/^(.+)\n([- ]+)$/ && length($1) == length($2))
		    {
		    print "<h4>", html($1), "</h4>\n";
		    next;
		    }
		if(/^(.+)\n([\. ]+)$/ && length($1) == length($2))
		    {
		    print "<h4>", html($1), "</h4>\n";
		    next;
		    }

		if(/^\* Menu:/)
		    {
		    $state = "menu";
		    print "<p>", html($_), "</p>\n";
		    print "<ul>\n";
		    next;
		    }

		# Preformatted
		# A preformatted section will be indented, usually by 5
		# spaces.
		#if(/^      *[^1-9\*\s]/)
		if(/^     /)
		    {
		    print "<pre>\n";
		    print html($_), "\n";
		    print "</pre>\n";
		    next;
		    }

		$_ = linkize($_);

		print "<p>$_</p>\n";
		next;
		}

	    #
	    # Menu:
	    #
	    # * Some Section::
	    # * '-w': Some Section.
	    #
	    if($state eq "menu")
		{
		s/\n[ \t]+/ /g;			# unwrap long lines
		foreach (split(/\n/))
		    {
		    # Ordinary menu entry.  It contains a linked section
		    # name.
		    if(/^\* ([^:]+)(::.*)$/)
			{
			my $name = $1;
			my $tail = $2;
			my $name_escaped = $name;
			$name_escaped =~ s/\s+/_/g;
			print "<li><a href=\"#$name_escaped\">", html($name), "</a>", html($tail), "\n";
			}
		    # Index menu entry.  It contains an index entry and a
		    # linked section name.
		    elsif(/^\* ([^:]+:)\s+(.+)\.$/)
			{
			my $head = $1;
			my $name = $2;
			my $name_escaped = $name;
			$name_escaped =~ s/\s+/_/g;
			print "<li>", html($head), " <a href=\"#$name_escaped\">", html($name), "</a>.\n";
			}
		    else
			{
			print "<p class=\"error\">", html($_), "</p>\n";
			}
		    }

		print "</ul>\n";
		$state = "normal";
		next;
		}

	    # We discard everthing before the first navigation point
	    next if($state eq "discard");

	    print "<p class=\"error\">", html($_), "</p>\n";
	    }

	close(DOC) || die $!;
	}
    }

    };
if($@)
    {
    my $message = $@;
    if($file_count > 0)
	{
	print "<p>", H_("Error: "), html($message), "</p>\n";
	}
    else
	{
	require "cgi_error.pl";
	error_doc("CGI Script Failed", ("<p>" . H_("Error: ") . html($message) . "</p>"), $charset, $content_language);
	exit 0;
	}
    }

print <<"EndOfTail";
</body>
</html>
EndOfTail

exit 0;
