#! /usr/bin/perl -wT
#
# mouse:~ppr/src/www/test_components.cgi.perl
# Copyright 1995--2004, Trinity College Computing Center.
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
# Last modified 15 April 2004.
#

use lib "?";
require 'paths.ph';
require 'cgi_data.pl';
require 'cgi_intl.pl';

#
# This is the path which we use for tests.  We have to provide one since the 
# web server protects us from such nasty things.
#
my $PATH = "/usr/local/bin:/bin:/usr/bin";

#
# This is a table of tests.
#
my @tests = (
		{'name' => _("PPR sound files"),
				'testproc' => sub {
						require "speach.pl";
						return speach_soundfiles_installed();
						},
				'required' => 0,
				},
		{'name' => _("Perl 5.005 or later"),
				'testproc' => sub {
						return ($] >= 5.005);
						},
				'required' => 1,
				'redhat' => "perl",
				'debian' => "perl-base",
				'source' => "http://www.cpan.org/src/stable.tar.gz"
				},
		{'name' => _("Perl GNU Gettext module"),
				'testproc' => sub {
						return defined(eval { require Locale::gettext });
						},
				'required' => 0,
				'debian' => "perl-tk"
				},
		{'name' => _("Perl Tk module"),
				'testproc' => sub {
						return defined(eval { require Tk });
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
		{'name' => _("Other Ghostscript distribution"),
				'testproc' => sub {
						return inpath("gs");
						},
				'required' => 0
				},
		{'name' => _("IJS Gimp Print drivers"),
				'testproc' => sub {
						return inpath("ijsgimpprint");
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
	my $class = shift;
	if(defined $text)
		{
		return "<td" . (defined($class) ? " class=$class" : "") . ">" . html_nb($text) . "</td>";
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
	foreach my $dir (split(/:/, $PATH))
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
TD.yes { color: green; }
TD.no  { color: red; }
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
	my $found = &{$test->{testproc}};
	my $found_text = $found ? _("Yes") : _("No");
	my $found_class = $found ? "yes" : "no";

	my $critical = $test->{critical};
	my $critical_text = $critical ? _("Yes") : _("No");
	my $critical_class = $critical ? "yes" : "no";
	
	print "<tr>", cell($test->{name}),
				cell($found_text, $found_class),
				cell($critical_text, $critical_class),
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
