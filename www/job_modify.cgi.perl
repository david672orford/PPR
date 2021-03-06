#! @PERL_PATH@ -wT
#
# mouse:~ppr/src/www/job_modify.cgi.perl
# Copyright 1995--2005, Trinity College Computing Center.
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
# Last modified 14 October 2005.
#

use lib "@PERL_LIBDIR@";
require "paths.ph";
require "cgi_data.pl";
require "cgi_tabbed.pl";
require "cgi_intl.pl";
require "cgi_run.pl";

defined($RESPONDERDIR) || die;
defined($PPOP_PATH) || die;

# Display a label and value.
sub text_display
	{
	my $label = html(shift);
	my $value = html(shift);
	my $width = shift;

	my $two_lines = chomp($label);

	print "<p><span class=\"label\">", html($label), "</span>\n";

	if($two_lines)
		{ print "<br>\n" }

	print "<span class=\"value\">", html($value);

	my $deficit = ($width - length($value));
	if($deficit < 0) { $deficit = 0 }
	$deficit = int($deficit * 1.5);				# fudge factor
	while($deficit--)
		{ print "&nbsp;" }

	print "</span></p>\n";

	}

# Display a name and editable value.
sub text_entry
	{
	my $label = html(shift);
	my $name = shift;
	my $value = html(shift);
	my $width = shift;
	my $maxwidth = shift;

	if(!defined($maxwidth)) { $maxwidth = 128 }

	my $two_lines = chomp($label);

	print "<p><span class=\"label\">", html($label), "</span>\n";

	if($two_lines)
		{ print "<br>\n" }

	print "<input type=\"text\" name=\"$name\" value=\"", html($value), "\" size=$width maxlength=$maxwidth></p>\n";
	}

my $tabbed_table = [
#=========================================
# General
#=========================================
{
'tabname' => N_("General"),
'dopage' => sub {
print "<p>", H_("This page gives general information about the print job."), "</p>\n";

text_display(_("Job ID:"), cgi_data_peek('jobname', ''), 20);
text_display(_("Submitted by:"), cgi_data_peek('for', ''), 37);
text_display(_("Submitted at:"), cgi_data_peek('longsubtime', ''), 20);
text_entry(_("Job title:"), "title", cgi_data_move('title', ''), 40);
text_display(_("Job creator:"), cgi_data_peek('creator', ''), 40);
}
},

#=========================================
# Ticket
#=========================================
{
'tabname' => 'Ticket',
'cellpadding' => 10,
'dopage' => sub {
		print "<p>", H_("This page contains instructions for a human printer operator.  Use\n"
				. "this page when the printer is located in a central location such as a\n"
				. "print shop."), "</p>\n";

		text_entry(_("Your name:"), "for", cgi_data_move("for", ""), 40);
		text_entry(_("Your department:"), "addon:department", cgi_data_move("addon:department", ""), 40);
		text_entry(_("Your telephone number:"), "addon:phone", cgi_data_move("addon:phone", ""), 12);

		text_entry(_("Account number:"), "addon:accountnumber", cgi_data_move("addon:accountnumber", ""), 12);
		text_entry(_("Date wanted:"), "addon:datewanted", cgi_data_move("addon:datewanted", ""), 12);
		text_entry(_("Delivery instructions (routing):") . "\n", "routing", cgi_data_move("routing", ""), 60);

		print "<p><span class=\"label\">", H_("Special processing instructions:"), "</span><br>\n";
		print "<textarea name=\"addon:specialprocessing\" cols=60 rows=6 wrap=\"physical\">\n";
		print html(cgi_data_move('addon:specialprocessing', ''));
		print "</textarea>\n";
		}
},

#=========================================
# Responder
#=========================================
{
'tabname' => 'Responder',
'dopage' => sub {
		print "<p>", H_("This page describes how the person who submitted the job will be notified\n"
				. "when it has been printed or when a problem arises."), "</p>\n";

		my $responder = cgi_data_move("responder", "");
		opendir(RD, $RESPONDERDIR) || die;
		print "<p><span class=\"label\">", H_("Responder method:"), "</span><select name=\"responder\">\n";
		while($_ = readdir(RD))
			{
			next if(/^\./);
			print "<option";
			print " selected" if($_ eq $responder);
			print ">$_\n";
			}
		closedir(RD) || die;
		print "</select>\n";

		text_entry(_("Responder address:") . "\n", "responder-address", cgi_data_move('responder-address', ''), 50);
		text_entry(_("Responder options:") . "\n", "responder-options", cgi_data_move('responder-options', ''), 60);
		}
},

#=========================================
# Proxy
#=========================================
{
'tabname' => 'Proxy',
'dopage' => sub {
		print "<p>", H_("This page gives precise and rather technical information about who entered\n"
				. "the job in the queue and on behalf of whom.");

		text_display(_("Submitter description:"), cgi_data_peek('for', ''), 30);
		text_display(_("Submitted by:"), cgi_data_peek('username', ''), 16);
		}
},

#=========================================
#
#=========================================
#{
#'tabname' => 'Blank1',
#'dopage' => sub {
#		print "<p>Not yet implemented.\n";
#		}
#},

#=========================================
#
#=========================================
#{
#'tabname' => 'Blank2',
#'dopage' => sub {
#		print "<p>Not yet implemented.\n";
#		}
#},

#=========================================
# The "ppop log" output
#=========================================
{
'tabname' => N_("Log"),
'cellpadding' => 10,
'dopage' => sub {
		print "<p>", H_("This page lists potential problems with this job.  If an attempt has already\n"
				. "been made to print it and the PostScript code wrote to the stdout or the stderr\n"
				. "of the PostScript interpreter, those messages may also appear here."), "</p>\n";

		opencmd(P, $PPOP_PATH, "-M", "log", cgi_data_peek('jobname', '?')) || die;
		print "<textarea rows=15 cols=70 wrap=\"off\" tabindex=0 readonly>\n";
		while(<P>)
			{
			print &html($_);
			}
		print "</textarea>\n";
		close(P) || die;
		}
}

];

#=========================================
# What are the field names?
#=========================================

# These are the fields we are going to retrieve.
my @read_only = qw(
		jobname longsubtime
		creator
		username
		);

my @read_write = qw(
		creator
		title
		for
		routing
		addon:department
		addon:phone
		addon:accountnumber
		addon:datewanted
		addon:specialprocessing
		responder responder-address responder-options
		commentary
		);

#=========================================
# This function load the job description.
#=========================================
sub load {
		my $jobname = $data{jobname};
		defined($jobname) || die "CGI variable \"jobname\" is undefined";

		# Set the query to ppop and get a list of values in return.
		opencmd(PPOP, $PPOP_PATH, "-M", "qquery", $jobname, @read_only, @read_write) || die "ppop failed, $!";
		my $result_line = <PPOP>;
		chomp $result_line;
		close(PPOP) || die "ppop failed: $result_line";
		my @result = split(/\t/, $result_line);

		foreach my $col (@read_only, @read_write)
			{
			$data{$col} = shift @result;
			}

		foreach my $item (@read_write)
			{
			$data{"_$item"} = $data{$item};
			}

		# Decode the special processing line.
		if(defined($data{'addon:specialprocessing'}))
			{
			$data{'addon:specialprocessing'} =~ s/\\n/\n/g;
			}
		}

#=========================================
# This function saves changes to the job.
#=========================================
sub save {
		my $jobname = $data{jobname};
		my $username = $ENV{REMOTE_USER};
		my $node = $ENV{REMOTE_ADDR};
		defined($jobname) || die "CGI variable \"jobname\" is undefined";
		defined($username) || die "CGI environment variable REMOTE_USER not defined";
		defined($node) || die "CGI environment variable REMOTE_ADDR not defined";

		# If the user is remote, so indicate.
		if(!getpwnam($username))
			{
			$username = "$username\@$node";
			}

		# Encode the special processing line.
		if(defined($data{'addon:specialprocessing'}))
			{
			$data{'addon:specialprocessing'} =~ s/\r//g;
			$data{'addon:specialprocessing'} =~ s/\n/\\n/g;
			}

		my @list = ();
		foreach my $item (@read_write)
			{
			if($data{$item} ne $data{"_$item"})
				{
				push(@list, "$item=$data{$item}");
				}
			}

		print "<p><b>", H_("Save changes to job:"), "</b><br>\n";
		print "<pre>\n";
		run($PPOP_PATH, "--user", $username, "modify", $jobname, @list);
		print "</pre>\n";
		}

#=========================================
# main
#=========================================

($<,$>) = ($>,$<);

&cgi_read_data();

&do_tabbed($tabbed_table,
		sprintf(_("PPR: Modify Job: %s"),
		$data{jobname}),
		\&load, \&save,
		7,
		{'height' => 400
				},
		);

exit 0;

