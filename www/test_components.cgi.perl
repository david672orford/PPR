#! /usr/bin/perl -wT
#
# mouse:~ppr/src/www/test_components.cgi.perl
# Copyright 1995--2002, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appears in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is"
# without express or implied warranty.
#
# Last modified 10 September 2002.
#

use lib "?";
require 'paths.ph';
require 'cgi_data.pl';
require 'cgi_intl.pl';

#
# This is a table of tests.
#
my @tests = (
		{'name' => _("Perl 5.005 or later"),
				'testproc' => sub {
						return ($] >= 5.005);
						},
				'required' => 1,
				'source' => "http://www.cpan.org/src/stable.tar.gz"
				},
		{'name' => _("PPR sound files"),
				'testproc' => sub {
						require "speach.pl";
						return speach_soundfiles_installed();
						},
				'required' => 0,
				},
		{'name' => _("Perl GNU Gettext module"),
				'testproc' => sub {
						return defined(eval { require Locale::gettext });
						},
				'required' => 0,
				'debian' => "liblocale-gettext-perl"
				},
		{'name' => _("NetPBM image converters"),
				'testproc' => sub {
						return inpath("pnmtops");
						},
				'required' => 0
				},
		{'name' => _("PPR-GS Ghostscript distribution"),
				'testproc' => sub {
						return -x "$HOMEDIR/../ppr-gs/bin/gs";
						},
				'required' => 0
				},
		{'name' => _("Groff"),
				'testproc' => sub {
						return inpath("groff");
						},
				'required' => 0
				},
		{'name' => _("HTMLDOC"),
				'testproc' => sub {
						return inpath("htmldoc");
						},
				'required' => 0
				},
		{'name' => _("Acroread"),
				'testproc' => sub {
						return inpath("acroread");
						},
				'required' => 0
				},
		{'name' => _("JPEG utilities"),
				'testproc' => sub {
						return inpath("djpeg");
						},
				'required' => 0
				},
);

sub cell
	{
	my $text = shift;
	if(defined $text)
		{
		return "<td>" . html_nb($text) . "</td>";
		}
	else
		{
		return "<td>&nbsp;</td>";
		}
	}

sub cell_url
	{
	my $text = shift;
	if(defined $text)
		{
		return "<td><a href=" . html_value($text) . ">" . html($text) . "</a></td>";
		}
	else
		{
		return "<td>&nbsp;</td>";
		}
	}

sub inpath
	{
	my $prog = shift;
	foreach my $dir (split(/:/, $ENV{PATH}))
		{
		return 1 if(-x "$dir/$prog");
		}
	return 0;
	}

#
# Create the top of the page.
#
my $title = H_("Inventory of Packages Which PPR can Use");
my ($charset, $content_language) = cgi_intl_init();
print <<"Quote10";
Content-Type: text/html;charset=$charset
Content-Language: $content_language
Vary: accept-language

<html>
<head>
<title>$title</title>
<style type="text/css">
BODY { background: white; color: black; }
TH, TD { text-align: left; padding: 0.5mm 1.0mm; }
</style>
</head>
<body>
<h1>$title</h1>
<table border=1 cellspacing=0>
Quote10

#
# Print the column headings.
#
{
print "<tr>";
my $i;
foreach $i (_("Component"), _("Present?"), _("Critical?"), _("Redhat Package Name"), _("Debian Package Name"), _("Source Download"))
	{
	print "<th>", html_nb($i), "</th>";
	}
print "</tr>\n";
}

#
# Run the tests and print the results.
#
{
foreach my $test (@tests)
	{
	my $found = &{$test->{testproc}} ? _("Yes") : _("No");
	my $critical = $test->{critical} ? _("Yes") : _("No");
	print "<tr>", cell($test->{name}),
				cell($found),
				cell($critical),
				cell($test->{redhat}),
				cell($test->{debian}),
				cell_url($test->{source}),
				"</tr>\n";
	}
}

#
# Close the HTML document.
#
print <<"Quote100";
</table>
</body>
</html>
Quote100

exit 0;
