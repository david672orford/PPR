#! /usr/bin/perl -wT
#
# mouse:~ppr/src/www/docs_man.cgi.perl
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
# Last modified 30 October 2001.
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

# Prevent taint problems when running gzip and such.  See Programming 
# Perl 3rd edition, p. 565.
defined($ENV{PATH} = $PPR::SAFE_PATH) || die;
delete @ENV{qw(IFS CDPATH ENV BASH_ENV)};

# Read POST and query variables.
&cgi_read_data();

# Initialize the internationalization libraries and determine the
# content language and charset.
my ($charset, $content_language) = cgi_intl_init();

# This first try block catches errors in operations which must be done before the
# HTTP header is generated.  If an error is caught, it produces an error document.
eval {
    my $path = docs_open(DOC, $ENV{PATH_INFO});
    $path =~ m#/([^/]+)\.(\d+[a-zA-Z]*)(\.[^\./]+)?$# || die;
    $html_title = html("$1($2)");
    docs_last_modified(DOC);
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

  while(my $line = <DOC>)
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
	    print "<h2>", troff_to_html(html($argument)), "</h2>\n";
	    }

	# If it is the start of a new paragraph,
	elsif($command eq "PP" || $command eq "P" || $command eq "LP")
	    {
	    print "<p>", troff_to_html(html($argument)), "\n";
	    }

	# If an indented paragraph,
	elsif($command eq "IP")
	    {
	    # This isn't right!!!
	    print "<p>", troff_to_html(html($argument)), "\n";
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
	    print "<br><b>.", $command, "</b> ", troff_to_html(html($argument)), "\n";
	    }
	}

    # If no dot command, just HTML escape <, >, and & and convert Troff
    # backslash commands to font changes.
    else
	{
	print troff_to_html(html($line)), "\n";
	}
    }

  close(DOC) || die $!;

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

# Convert Troff font changes to HTML font changes.
sub troff_to_html
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

# Display words in alternate fonts.
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

