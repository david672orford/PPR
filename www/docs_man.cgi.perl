#! /usr/bin/perl -wT
#
# mouse:~ppr/src/www/docs_man.cgi.perl
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
# Last modified 18 January 2003.
#

#
# This CGI program reads a plain text file and converts it to HTML.  While
# doing so, anything that looks like a URL or an email address is converted
# into a link.
#

use lib "?";
use PPR;
require "cgi_data.pl";
require "cgi_intl.pl";
require "docs_util.pl";

@MANPATH = ("/usr/local/man", "/usr/share/man", "/usr/man");

# Prevent taint problems when running gzip and such.  See Programming
# Perl 3rd edition, p. 565.
defined($ENV{PATH} = $PPR::SAFE_PATH) || die;
delete @ENV{qw(IFS CDPATH ENV BASH_ENV)};

# This function search the MANPATH.
sub search_manpath
    {
    my $manpath = shift;
    my $name = shift;
    my $section = shift;

    foreach my $dir (@{$manpath})
	{
	next if(! -d $dir);		# skip MANPATH directories that don't exist

	# If the user has specified a section, look in that.  Otherwise
	# scan and figure out which sections exist.
	my @sections;
	if(defined $section)
	    {
	    @sections = ($section);
	    }
	else
	    {
	    @sections = ();
	    opendir(DIR, $dir) || die $!;
	    while(my $d = readdir(DIR))
		{
		if($d =~ /^man(.+)$/)
		    {
		    push(@sections, $1);
		    }
		}
	    closedir(DIR) || die $!;
	    @sections = sort(@sections);
	    }

	foreach my $sect (@sections)
	    {
	    my $sectdir = "man$sect";
	    foreach my $compext ("", ".gz", ".bz2")
		{
		# Try looking it up the a straightforward way.
		my $filename = "$dir/$sectdir/$name.$sect$compext";
		return $filename if(-f $filename);

		# Debian will put Perl's Chart(3) in /usr/share/man/man3/Chart.3pm.gz.
		# This will find it is the user specified it as "Chart(3pm)".
		if($sectdir !~ /[0-9]$/ && ! -d $sectdir)
		    {
		    $sectdir =~ s/[^0-9]+$//;
		    $filename = "$dir/$sectdir/$name.$sect$compext";
		    return $filename if(-f $filename);
		    }

		# This will find Perl's Chart(3) if the user specified it as
		# "Chart" or "Chart(3)".
		$filename = glob("$dir/$sectdir/$name.$sect*$compext");
		return $filename if(-f $filename);
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
    cgi_redirect("http://$ENV{SERVER_NAME}:$ENV{SERVER_PORT}$ENV{SCRIPT_NAME}/MANPATH/$document");
    }

# Initialize the internationalization libraries and determine the
# content language and charset.
my ($charset, $content_language) = cgi_intl_init();

# This first try block catches errors in operations which must be done before
# the HTTP header is generated.  If an error is caught, it produces an error
# document.
my $path = undef;
eval {

    # If this script was invoked as if it were a directory,
    if(defined($ENV{PATH_INFO}))
	{
	# If the path begins with "/MANPATH/", find the man page
	# in the MANPATH, just like man(1) would.
	if($ENV{PATH_INFO} =~ m#/MANPATH/(.+)#)
	    {
	    my $page = $1;
	    $page !~ m#/# || die "No subirectories within MANPATH";
	    $page =~ m#(^[^\(]+)(\(([^\)]+)\))?$# || die "Invalid manpage name \"$page\"";
	    my($name, $section) = ($1, $3);
	    $path = search_manpath(\@MANPATH, $name, $section);
	    defined($path) || die "Can't find $name($section) in MANPATH";
	    }

	# Otherwise, accept the path that the user supplies.
	else
	    {
	    $path = $ENV{PATH_INFO};
	    }

	my $cleaned_path = docs_open(DOC, $path);
	$cleaned_path =~ m#/([^/]+)\.(\d+[a-zA-Z]*)(\.[^\./]+)?$# || die "Filename \"$cleaned_path\" is not that of a man page";
	$html_title = html("$1($2)");
	docs_last_modified(DOC);
	}

    else
	{
	$html_title = html(_("Available Manpages"));
	}

    };
if($@)
    {
    my $message = $@;
    require 'cgi_error.pl';
    error_doc(_("Can't Display Manpage"), html($message), $charset, $content_language);
    exit 0;
    }

print <<"EndOfHead";
Content-Type: text/html

<html>
<head>
<title>$html_title</title>
</head>
<body>
EndOfHead

# This is a second try block to catch errors that occur while
# generating the body.  It aborts the processing of the man
# page text and prints an error message as a paragraph.
eval {

    if(defined $path)
	{
	print "<p>Help the PPR project!  If you have documentation\n";
	print "for any untranslated macros below, send it to \n";
	print "<a href=\"mailto:ppr-bugs@mail.trincoll.edu\">ppr-bugs\@mail.trincoll.edu</a>.</p>\n";

	troff_to_html(DOC);
	close(DOC) || die $!;
	}
    else
	{
	print "<h1>$html_title</h1>\n";
	do_man_directory(\@MANPATH);
	}

    };
if($@)
    {
    my $message = $@;
    print "<p>", H_("Error:"), " ", html($message), "</p>\n";
    }

print <<"EndOfTail";
</body>
</html>
EndOfTail

exit 0;

#=============================================================================
# Directory Generation Routines
#=============================================================================

sub do_man_directory
    {
    my $manpath = shift;
    my %sections;
    foreach my $dir (@$manpath)
	{
	next if(! -d $dir);	# skip MANPATH directories that don't exist

	opendir(DIR, $dir) || die $!;
	while(my $dent = readdir(DIR))
	    {
	    if($dent =~ /^man(.+)$/ && -d "$dir/$dent")
		{
		my $section_number = $1;
		opendir(SDIR, "$dir/$dent") || die $!;
		while($sdent = readdir(SDIR))
		    {
		    if($sdent =~ /^([^\.]+)\./)
			{
			if(! defined $sections{$section_number})
			    {
			    $sections{$section_number} = [];
			    }
			push(@{$sections{$section_number}}, ["$1($section_number)", "$dir/$dent/$sdent"]);
			}
		    }
		closedir(SDIR) || die $!;
		}
	    }
	closedir(DIR) || die $!;
	}

    foreach my $key (sort keys %sections)
	{
	print "<h2>", html(sprintf(_("Section %s"), $key)), "</h2>\n";
	print "<ul>\n";
	for my $i (sort {$a->[0] cmp $b->[0]} @{$sections{$key}})
	    {
	    print "<li><a href=\"$ENV{SCRIPT_NAME}$i->[1]\">", html($i->[0]), "</a></li>\n";

	    }
	print "</ul>\n";

	}
    }

#=============================================================================
# Troff Processing Routines
#=============================================================================

sub troff_to_html
  {
  my $doc = shift;

  while(my $line = <$doc>)
    {
    chomp $line;		# remove newline
    $line =~ s/\s+$//;		# remove confusing trailing whitespace

    # For debugging.
    #print "<font color=green><pre>", html($line), "</pre></font>\n";

    # If the line starts with a Troff dot command,
    if($line =~ /^\.(\S{1,2})(\s+(.+))?$/)
	{
	my($command, $argument) = ($1, $3);
	$argument = "" if(! defined $argument);

	# If it is a comment,
	if($command eq "\\\"")
	    {
	    next;
	    }

	# If it is the main title,
	elsif($command eq "TP" || $command eq "TH")
	    {
            if($argument =~ /^(\S+)\s+(\S+)(?:\s+"([^"]*)")?(?:\s+"([^"]*)")?(?:\s+"([^"]*)")?$/)
		{
		my($name, $section, $date, $package, $vendor) = ($1, $2, $3, $4, $5);
		print "<h1>", html($name), "(", html($section), ")</h1>\n";
		}

	    # Try simpler,
	    elsif($argument =~ /^(\S+)\s+(\S+)/)
	    	{
		my($name, $section) = ($1, $2);
		print "<h1>", html($name), "(", html($section), ")</h1>\n";
	    	}

	    # If still can't parse it,
	    else
	    	{
	    	print "<h1>", html($argument), "</h1>\n";
	    	}
	    }

	# If it is a section heading,
	elsif($command eq "SH")
	    {
	    $argument =~ s/^"(.*)"$/$1/;
	    print "<h2>", troff_escapes_to_html(html($argument)), "</h2>\n";
	    }

	# If it is the start of a new paragraph,
	elsif($command eq "PP" || $command eq "P" || $command eq "LP")
	    {
	    print "<p>", troff_escapes_to_html(html($argument)), "\n";
	    }

	# If an indented paragraph,
	elsif($command eq "IP")
	    {
	    # This isn't right!!!
	    print "<p>", troff_escapes_to_html(html($argument)), "\n";
	    }

	# If argument is to be bolded,
	elsif($command eq "B")
	    {
	    print "<b>", html($argument), "</b>\n";
	    }

	# If argument is to appear alternately in bold and roman,
	elsif($command eq "BR")
	    {
	    alternate($argument, "<b>", "</b>", "", "");
	    }

	# If alternating italic and roman,
	elsif($command eq "IR")
	    {
	    alternate($argument, "<i>", "</i>", "", "");
	    }

	# I think this is a line break.
	elsif($command eq "br" && $argument eq "")
	    {
	    print "<br>\n";
	    }

	# We can't figure it out.  Just print the command.
	else
	    {
	    print "<br><b>.", $command, "</b> ", troff_escapes_to_html(html($argument)), "\n";
	    }
	}

    # If no dot command, just HTML escape <, >, and & and convert Troff
    # backslash commands to font changes.
    else
	{
	print troff_escapes_to_html(html($line)), "\n";
	}
    }
  } # end of troff_to_html()

# Convert Troff font changes to HTML font changes.
sub troff_escapes_to_html
    {
    my $text = shift;

    # Font changes:
    $text =~ s/\\fI(.+?)\\fR/<i>$1<\/i>/g;
    $text =~ s/\\fB(.+?)\\fR/<b>$1<\/b>/g;

    # Special characters:
    $text =~ s/\\\(co/&copy;/g;
    $text =~ s/\\-/-/g;

    return $text;
    }

# Display words in alternating fonts.
sub alternate
    {
    my($argument, $first_open, $first_close, $second_open, $second_close) = @_;
    my $x = 0;
    foreach my $word (split(/\s+/, $argument))
        {
        if(($x++ % 2) == 0)
            { print $first_open, html($word), $first_close }
        else
            { print $second_open, html($word), $second_close }
        }
    print "\n";
    }

# end of file
